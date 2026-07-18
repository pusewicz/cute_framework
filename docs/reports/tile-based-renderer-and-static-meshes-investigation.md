# Tile-Based Rendering and Static Meshes: Investigating Randy Gaul's Perf Claim

**Date:** 2026-07-18
**Branch:** `master` @ `bb3a0aee`
**Scope:** Investigate the claim (relayed by the user, attributed to CF author Randy Gaul):
*"I do think switching to a tile based renderer would probably be more important though to
get more perf benefits tho, and also static meshes"* — against CF's current draw
architecture, and against the optimization work already scoped in
[`docs/superpowers/specs/2026-07-18-draw-batcher-churn-and-instancing-design.md`](../superpowers/specs/2026-07-18-draw-batcher-churn-and-instancing-design.md)
and [`approach-b-ssbo-vertex-pulling-feasibility.md`](./approach-b-ssbo-vertex-pulling-feasibility.md).

**A note on sourcing:** I could not find a public record of this exact statement — a search
of `RandyGaul/cute_framework` issues (titles: "tile based renderer", "static mesh", "tile",
"batch") and PRs turned up nothing matching, and the repo has GitHub Discussions disabled.
I'm treating it as context relayed directly by the user (presumably from a private
conversation) rather than something I can cite a source for. Everything below is my own
analysis of the technical claim against the actual code, not a summary of Randy's reasoning
— I don't have his reasoning to summarize.

---

## Executive summary

The claim decomposes into two related but distinct ideas, and both point at a **structural
gap that is almost certainly larger than anything items 2/3 or Approach B address**: CF's
draw pipeline today has **zero frustum/viewport culling** and **zero static-content
caching**. Every `cf_draw_*` call — on-screen or not, changed since last frame or not — pays
the full CPU vertex-generation + spritebatch cost, every single frame. Items 2 and 3 (and
the rejected Approach B) all optimize *how fast* CF processes each draw call; they do
nothing to reduce *how many* draw calls get processed, or whether they needed to be
processed again at all.

| Finding | Status |
|---|---|
| CPU-side frustum/viewport culling in `cute_draw.cpp` | **Does not exist** — confirmed by grep, zero matches |
| Static/unchanged-content caching (skip re-batching if nothing changed) | **Does not exist** — every batch is rebuilt every frame regardless |
| Render-to-texture / canvas baking primitive, usable for "static meshes" today | **Exists and is proven** — `cf_make_canvas`/`cf_render_to`/`cf_draw_canvas`, shipped, sampled, one bugfix PR merged |
| Low-level persistent-mesh API, usable for "static meshes" bypassing cute_draw | **Exists and is proven** — `CF_Mesh` public API, used internally by cute_draw itself |
| Compute-shader + storage-buffer infrastructure (a natural base for tile binning) | **Exists and is proven for compute** — unlike the graphics-stage SSBO gap found for Approach B |
| A full tile-based (spatially binned) rendering architecture | **Does not exist** — would be a substantial rewrite of the draw pipeline, not a spike |

**Bottom line:** Randy's suggestion is credible and probably right that it's "more
important" in the sense of larger potential impact, *for scenes shaped like typical
tile-heavy 2D games* — but it's two different-sized pieces of work. "Static meshes" (in the
canvas-baking sense) is close to free — CF already has the primitive, shipped and proven;
someone just has to use/document it for this purpose. A genuine tile-based renderer is a
multi-week-plus architectural rewrite, not something to scope alongside items 2/3 in the
same session. Missing frustum culling, which neither the design doc nor Approach B mentions
at all, may be the single highest-leverage and cheapest fix of everything discussed so far —
see §5.

---

## 1. What "tile-based renderer" most plausibly means here

The term is ambiguous without more from Randy, so I'm laying out the reading that's most
consistent with pairing it with "static meshes" in the same sentence, and with what CF is
actually missing: organize drawable content into spatial units (screen-space tiles and/or
world-space chunks), so the renderer can (a) skip tiles that don't overlap the camera
(culling) and (b) treat tiles whose content hasn't changed as cacheable/static rather than
rebuilding them every frame. Under this reading, "tile-based renderer" and "static meshes"
aren't two unrelated ideas — they're the same underlying fix (stop treating every draw call
as independent, always-visible, and always-dirty) approached from two angles: the rendering
architecture (tiles/bins) and the content model (persistent geometry for things that don't
move).

A narrower reading — tile-based rendering as in mobile GPU tile-based deferred rendering
(TBDR), i.e. binning primitives into GPU-native tiles to reduce overdraw/fill-rate cost for
CF's SDF-shape fragment shader — is also plausible and not mutually exclusive; see §4.

---

## 2. What's actually missing today: culling

`src/cute_draw.cpp` has no viewport/frustum culling of any kind. A grep for
`cull|frustum|visible|onscreen` across the file turns up exactly one unrelated use of
`visible` (glyph emptiness for text rendering, `src/cute_draw.cpp:2004,2048,2838` — whether
a font glyph has any ink, not whether it's on-screen). There is no AABB-vs-viewport check
anywhere in the `cf_draw_sprite`/`cf_draw_quad`/`cf_draw_box` family, and no such check in
`s_draw_report` or the spritebatch input path either.

**Concretely:** if a game draws a 200×200-tile static level (40,000 tiles) and only ~50×30
tiles (1,500) are ever visible on screen at once, CF today does full CPU vertex generation,
spritebatch input processing, and GPU vertex upload for **all 40,000 tiles, every frame** —
unless the game itself hand-rolls a visibility check before calling `cf_draw_sprite`. That's
roughly a 26× waste in this illustrative example, and it compounds directly with everything
items 2/3 and Approach B are trying to speed up: culling reduces the *count* of items
flowing into the pipeline those items make faster. A cheap, CPU-side AABB-vs-camera-viewport
check before a draw call enters the command stream (or before it's added to spritebatch
input) would be a small, self-contained, low-risk change relative to any of the batcher
work — and unlike items 2/3, its win scales with how much of the world is off-screen, which
for most real 2D games (side-scrollers, top-down levels, large tilemaps) is often the
majority of drawn content today.

I'm not attaching a benchmark number to this — no measurement was taken, since it wasn't in
scope for this investigation — but the order of magnitude here is a different category from
the ~2×–12× wins already measured for `spritebatch-perf`, because it scales with world size
vs. viewport size, which is unbounded, rather than with per-primitive constant-factor cost.

---

## 3. Static meshes: the primitive already exists, twice

### 3.1 Canvas/render-to-texture baking (higher-level, ergonomic, already shipped)

`CF_Canvas` (`include/cute_graphics.h:69-1241`) is CF's offscreen render-target API:
`cf_make_canvas`, `cf_render_to(canvas, clear)`, `cf_draw_canvas(canvas, pos, size)`. It's
not experimental — it has a working sample (`samples/draw_to_texture.c`) and a merged
bugfix (`PR #401`, "Draw to texture fix", 2025-10-14). The sample's loop:

```c
// samples/draw_to_texture.c:19-25
cf_draw_circle2(cf_v2(0,0), 100, 5);
cf_render_to(offscreen, true);              // bakes queued draw calls into `offscreen`
cf_draw_canvas(offscreen, cf_v2(-w*0.25f,0), cf_v2(w*0.5f,h*0.5f));  // draws the baked texture as a quad
cf_draw_canvas(offscreen, cf_v2( w*0.25f,0), cf_v2(w*0.5f,h*0.5f));
```

This is exactly the primitive a "static mesh"/tile-chunk-caching system needs: render a
region of static content into a canvas once, then every subsequent frame just call
`cf_draw_canvas` — a single textured-quad draw — instead of re-issuing however many
individual `cf_draw_sprite` calls that region used to take. For a static tilemap chunk of,
say, 32×32 tiles, that's turning 1,024 draw calls (each paying full CPU vertex-gen +
spritebatch cost) into 1.

**What CF does *not* provide:** any notion of "this content is static, don't redo the
bake." The sample above calls `cf_render_to` every single frame — it demonstrates
render-to-texture as a compositing primitive, not caching. Getting the caching win requires
the game to only call `cf_render_to` when the chunk's content actually changed (a dirty
flag), which CF has no built-in support for — no chunk manager, no invalidation tracking,
no "has this region changed since I last baked it" API. That logic is entirely on the game
today.

**Real trade-offs worth naming**, since this isn't free:
- Baking to a fixed-resolution texture and then scaling it as a quad can blur or alias at
  camera zoom levels different from the bake resolution — mipmapping helps but this is a
  genuine quality trade-off vs. CF's current approach, where every shape is recomputed via
  SDF math at native resolution regardless of zoom.
- Chunk granularity, VRAM cost of many canvas render targets, and visible-chunk
  streaming/loading are all left to the game; CF provides the render-target primitive, not
  a chunk-management system.

### 3.2 Low-level persistent mesh (lower-level, more manual, also already shipped)

Separately, `CF_Mesh` (`include/cute_graphics.h`, `cf_make_mesh` /
`cf_mesh_update_vertex_data` / `cf_apply_mesh`) is CF's public low-level vertex-buffer API —
the same one `cute_draw.cpp` itself uses internally (`s_draw->mesh`, see
`src/cute_draw.cpp:369-370`). Nothing prevents a game from building its own `CF_Mesh` once
for static content and re-issuing `cf_apply_mesh` + a draw call every frame without going
through `cute_draw.cpp`'s per-frame CPU vertex generation or spritebatch at all — this is a
literal "static mesh" in the traditional sense (persistent GPU buffer, not a baked texture).

The catch: this bypasses everything `cute_draw.cpp` automates — atlas/UV assignment via
spritebatch, the SDF shape math, camera/transform-stack integration, text layout. A game
doing this has to hand-roll material/shader/uniform setup itself. It's a real, working path
today, but it's not integrated with the high-level `cf_draw_*` API a typical CF user
actually reaches for — which is likely the gap Randy's comment is pointing at: this
capability should probably be a first-class, ergonomic feature of `cute_draw.cpp` (e.g. "add
this batch of sprites as a static/cached group"), not something only reachable by dropping
to the low-level graphics API.

---

## 4. Tile-based rendering architecture: a much bigger undertaking

Reading "tile-based renderer" as screen-space spatial binning (closer to how mobile TBDR
GPUs or modern "visibility buffer"/tiled-forward renderers work): divide the screen into
fixed tiles, bin primitives per-tile (typically via a compute pre-pass), then shade
per-tile. For CF specifically, this would help most with **overdraw and fill-rate cost from
overlapping SDF shapes** — CF's fragment shader (documented in
[`approach-b-ssbo-vertex-pulling-feasibility.md`](./approach-b-ssbo-vertex-pulling-feasibility.md)
§4) does a real SDF distance evaluation plus blend-mode math per fragment, per overlapping
shape; a tile-based binning pass could skip evaluating primitives that don't overlap a given
tile, and could cull off-screen tiles entirely (folding in the culling win from §2 at the
rendering-architecture level instead of the CPU-draw-call level).

**This is not a spike-sized task.** It would mean replacing or substantially restructuring
the entire draw pipeline: spritebatch-based atlas batching, the CPU vertex-generation loop,
`s_draw_report`, and the built-in shaders — essentially everything items 2/3 and Approach B
touch, at once, plus new compute-shader binning logic. One relevant piece of good news from
the Approach B investigation: CF's compute-shader + storage-buffer infrastructure **is**
mature and cross-backend-proven for compute dispatch (used in `samples/hrc.c`,
`samples/galaxy.c`) — the gap found in that report was specifically about binding storage
buffers to *graphics* (vertex/fragment) stages, which a compute-driven tile-binning pass
doesn't need in the same way (it would write results to a texture or a simpler
already-supported binding, not require per-vertex SSBO reads). So a tile-based renderer is
architecturally more compatible with what CF already has proven than Approach B was — but
it's still a from-scratch rendering-pipeline design and implementation effort, multi-week
scope at minimum, not something to attempt alongside the current item 2/3 work.

---

## 5. How this reframes priorities

Items 2 and 3 (and the rejected Approach B) remain valid, real optimizations — they reduce
the fixed and per-primitive cost of processing whatever draw calls the pipeline actually
receives. They matter most for content that's genuinely dynamic and always visible
(moving entities, particles, UI). But for a large or mostly-static scene — which is exactly
what tile-based 2D games are — the ceiling on those items' impact is capped by two things
neither of them touches:

1. **CF processes every draw call regardless of visibility** (§2) — no culling exists at
   all today. This is likely the single highest-leverage, lowest-risk fix available: a
   simple AABB-vs-viewport check is a small, self-contained change, doesn't touch the
   batcher/spritebatch internals items 2/3 are modifying, and its win scales with world
   size rather than being capped at a small constant factor.
2. **CF re-processes every draw call every frame even if nothing changed** (§3) — the
   canvas-baking primitive to fix this for static content already exists and is proven; it
   mainly needs a documented pattern (or small first-class API) for dirty-tracked
   chunk/region caching, not new engine plumbing.

Neither of these was in scope for the item 2/3 design doc, and Approach B doesn't touch them
either — all three are about processing each draw call faster, not about processing fewer
of them or processing them less often. Given the magnitude difference (culling scales with
world-vs-viewport ratio, which is unbounded, vs. the ~2×–6× constant-factor wins already
measured or targeted), it's worth treating "add frustum culling" and "document/prototype
canvas-based static-chunk caching" as candidate follow-up work, likely higher-leverage than
Approach B was and plausibly comparable to or larger than items 2/3 for realistic tile-heavy
scenes — worth discussing as next steps once items 2/3 land.

---

## 6. Conclusion

Randy's claim holds up under investigation, with a caveat: "static meshes" is close to a
free win — the primitive (`CF_Canvas`) already exists, is shipped, and just needs a caching
pattern built on top; a genuine "tile-based renderer" is a real, large architectural
rewrite that shouldn't be scoped into the current session's work. The most concrete and
actionable finding from this investigation, though, is one neither the original claim nor
the existing design doc named directly: **CF has no frustum culling at all**, which for
any game with a world larger than its viewport is probably a bigger lever than any of the
per-primitive batcher optimizations under discussion.

---

## Appendix A — Source anchors

```
src/cute_draw.cpp                        — no cull/frustum/visible-viewport logic anywhere (grep-verified)
src/cute_draw.cpp:2004,2048,2838          — the only `visible` hits: font glyph emptiness, unrelated to culling
include/cute_graphics.h:69-1241           — CF_Canvas: cf_make_canvas, cf_render_to, cf_draw_canvas, cf_apply_canvas
samples/draw_to_texture.c                 — working render-to-texture sample (bakes every frame, not cached)
samples/canvas_readback.c                 — related canvas sample (CPU readback, not directly relevant here)
src/cute_draw.cpp:369-370                 — cute_draw's own internal use of CF_Mesh (s_draw->mesh)
include/cute_graphics.h (CF_Mesh section) — public low-level mesh API, usable standalone for static geometry
approach-b-ssbo-vertex-pulling-feasibility.md §3, §6 — compute/storage-buffer maturity vs. graphics-stage gap
GitHub issue #109 (RandyGaul/cute_framework, closed) — origin of the current SDF-based draw
  API ("major batch optimization possibilities"), useful context for why today's architecture
  looks the way it does
```

---

*End of report.*
