# Draw batcher follow-ups: churn overhead, per-quad instancing, and frustum culling

## Context

Prior work (2026-07-18, `spritebatch-perf` worktree, branch `spritebatch-perf`, 4 commits
ahead of `master` @ `bb3a0aee`, not yet merged) fixed the dominant CPU cost inside
`cute_spritebatch`'s internal sort/merge for already-ordered input, plus geometry-by-pointer
indirection and an image-resolution memo cache. Net effect: pure spritebatch CPU cost at
10k sprites/frame dropped ~12x (1242µs → ~105µs).

That work exposed two further, distinct hot paths that it did not address, both already
scoped in project memory:

1. **Per-batch fixed overhead in `s_draw_report`** (`src/cute_draw.cpp:123-412`). Every
   spritebatch-produced batch — regardless of how many sprites it contains — pays a full
   sequence: vertex build, `cf_mesh_update_vertex_data` + `cf_apply_mesh`, texture bind,
   4x `cf_material_set_uniform_fs`, `cf_material_set_render_state`, `cf_set_sampler_override`,
   `cf_apply_shader`, viewport apply, scissor apply, `cf_draw_elements`. Well-batched scenes
   amortize this over thousands of sprites/batch; any per-object state change (scissor,
   shader, render state, viewport, layer, alpha_discard, filter_mode — anything that trips
   `add_cmd()` in `cute_draw_internal.h:113-130`) breaks the batch and pays this cost per
   object. `draw_bench`'s `churn` kind (`samples/draw_bench.c`, pathological: push/pop scissor
   around every quad) measures ~0.77ms per broken batch. This was flagged as unexplored.

2. **Per-quad CPU vertex cost at high N.** `CF_Vertex` (`include/cute_draw.h:594`) is
   ~152 bytes; CF currently expands every quad/sprite into 6 fully CPU-computed vertices
   (~912 bytes/quad) before upload. The separate `batcher-refactor` worktree (branch
   `batcher-refactor`, Phase 1 commit `74fcbba2`) already did smaller surgical fixes here
   (binding `BatchGeometry` by reference, caching unchanged uniforms, `realloc`-based
   spritebatch buffer growth) and explicitly deferred "the real perf lever" — per-quad
   instancing, ~6x less vertex upload — to a Phase 2 that hasn't started.

**Key finding from this investigation:** CF's built-in draw vertex shader
(`src/data/builtin_shaders_bytecode.h`) is a pure passthrough — it forwards attributes
and sets `gl_Position` directly from a CPU-precomputed homogeneous position. All
quad-corner expansion, transform application, and SDF setup happens on the CPU, per
vertex, today. This means "per-quad instancing" is not a data-repacking exercise — it
requires writing a new vertex shader that does real GPU-side work (corner expansion from
a static unit quad + per-instance transform), because that's where the CPU cost is
currently hiding.

A follow-up investigation (`docs/reports/tile-based-renderer-and-static-meshes-investigation.md`,
prompted by a claim relayed from CF author Randy Gaul that a tile-based renderer and static
meshes would be a bigger perf lever) surfaced a third, independent gap:

3. **No frustum/viewport culling anywhere in `cute_draw.cpp`.** Every `cf_draw_*` call is
   processed at full cost — CPU geometry build, spritebatch input, vertex upload — whether
   or not it's on-screen. Unlike items 1/2/3, which reduce the *cost per draw call*, this
   reduces the *number* of draw calls that need processing at all, and its potential impact
   scales with world-size-vs-viewport ratio rather than being capped at a small constant
   factor. Investigating this further (see §Item 4) found that CF already computes each
   item's clip-space corners before pushing it into the batch — `BatchGeometry`'s
   `shape[]`/`box[]`/`boxH[]` fields are populated via `s_draw->mvp` (camera × projection)
   *before* `DRAW_PUSH_ITEM` runs (e.g. `src/cute_draw.cpp:602-611`) — so a visibility test
   at that single choke point needs no new transform math, just a bounds check on data
   that's already computed.

## Goals

- Fix or clearly characterize the `s_draw_report` per-batch fixed-cost problem (item 2).
- Build a scoped, measurable prototype of per-quad GPU instancing for plain textured
  quads/sprites (item 3), reusing CF's existing public instancing API
  (`cf_mesh_set_instance_buffer` / `cf_mesh_update_instance_data` /
  `CF_VertexAttribute::per_instance`), which is already on `master` and currently unused
  by `cute_draw.cpp`.
- Add CPU-side frustum/viewport culling to `cute_draw.cpp` (item 4) at the single choke
  point every draw call already passes through (`DRAW_PUSH_ITEM`), using clip-space bounds
  CF already computes per item, so off-screen content stops paying for CPU geometry build,
  spritebatch input, and vertex upload it doesn't need.
- Produce independent before/after `draw_bench` numbers for each item, using the project's
  established methodology (interleaved A/B binaries, min-of-rounds p50 — frame times on
  this Mac are bimodal due to P-core/E-core/compositor throttling).

## Non-goals

- Merging `spritebatch-perf`, `batcher-refactor`, or any output of this work into `master`.
  This is investigation + prototyping; merge decisions come later.
- Full SSBO vertex-pulling for all shape types (circles, polygons, capsules, text) —
  considered as "Approach B" and rejected as too large for this scope. Item 3 only
  covers plain textured quads/sprites; everything else keeps using the current CPU-expansion
  path unchanged, selected by falling back whenever a batch contains anything other than
  simple sprite/quad geometry.
- Shipping the item-3 prototype as production-ready public API. It's a spike to validate
  the instancing hypothesis and get real numbers; productionizing (handling all shape
  types, edge cases, mixed batches) is future work if the numbers justify it.

## Workstream setup

- **Item 2** gets its own new worktree, `../cute_framework-draw-report-overhead`, branch
  `draw-report-overhead`, branched from `master` @ `bb3a0aee` — *not* stacked on
  `spritebatch-perf`. `spritebatch-perf` (commit `b4da6836`) already refactored
  `s_draw_report`, splitting the exact block item 2 targets (vertex_fn, mesh upload,
  texture/uniform/render-state/sampler/shader/viewport/scissor apply, draw call) into a new
  `s_draw_report_submit` helper. Building item 2 from `master` instead keeps its diff
  independent of that refactor and its baseline the same `s_draw_report` shape already cited
  in this doc (`src/cute_draw.cpp:123-412`). The two branches will very likely conflict in
  this region if both land — that's a rebase to do at merge time, not a reason to couple the
  two pieces of work now.
- **Item 3** gets a new worktree, `../cute_framework-instanced-quads`, on a new branch
  `instanced-quads` branched from `spritebatch-perf`'s current tip (`1d9ca877`), keeping its
  baseline the same clean spritebatch-perf tip already used for the item-1 numbers.
  `batcher-refactor` is referenced for direction (its Phase 1 commit message and its
  already-landed P3/P5/P7 fixes) but not merged in — its diff also removes samples/tests
  unrelated to this work.
- **Item 4** also gets its own worktree, `../cute_framework-frustum-culling`, branch
  `frustum-culling`, branched from `spritebatch-perf`'s tip for the same reason as item 3
  (independent before/after comparison against the same clean base). Culling is orthogonal
  to items 2 and 3 — it reduces *how many* items reach the batcher, not how each one is
  processed — so it doesn't need either of their changes as a prerequisite, but shares their
  base branch for comparison consistency and because a future combined-stack measurement
  (culling + instancing together) is a natural next step once all three land independently.

## Item 2: `s_draw_report` per-batch overhead

**Tooling.** `samples/draw_bench.c` (the `churn` kind) and `samples/draw_soa_microbench.c`
(the phase-timer style this section reuses) only exist on `spritebatch-perf`, not `master`.
Both are self-contained samples built purely against public `cf_draw_*`/`cf_render_to` API —
neither depends on `spritebatch-perf`'s internal engine changes — so cherry-pick commits
`ba4be2b2` (draw_bench) and `1d9ca877` (draw_soa_microbench) into the item-2 worktree before
starting. This brings in bench tooling only, not any of the spritebatch optimization commits.

**Instrumentation.** Add `CF_Stopwatch`-based phase timers around each sub-call inside
`s_draw_report`, matching the phase-timer style in the cherry-picked `draw_soa_microbench`
sample (split submit/batch/present timings). This is throwaway diagnostic instrumentation —
it doesn't need to be production-quality and can be dropped or left behind a debug flag
once item 2 concludes. Run `draw_bench churn` (capped at N=2000 by the existing bench code)
with instrumentation active; take medians per phase across interleaved runs.

**Leading hypothesis (to confirm or refute with the data, not to implement blind):** most
of the `apply_X` backend calls re-set GPU state unconditionally every batch even when the
value is identical to what's already bound — `batcher-refactor`'s Phase 1 (P5) already did
this caching for the 4 material uniforms, but not for texture bind, render state, sampler
override, shader, or viewport. If the phase breakdown confirms one or more of these as the
dominant cost, add last-applied-value caching on `CF_Draw` and skip the redundant call,
following the same pattern P5 already established. If the breakdown instead points to
vertex upload (`cf_mesh_update_vertex_data`) or draw submission (`cf_draw_elements`) as
dominant, the fix target shifts accordingly — this design intentionally does not lock in
a specific code change ahead of measurement.

**Success criteria:** phase breakdown for `churn` published (which sub-call(s) dominate the
~0.77ms/batch), a fix implemented for whatever dominates, and a before/after `churn`-kind
`draw_bench` comparison at the existing N.

## Item 3: scoped per-quad instancing prototype

**Scope.** Plain textured quads and sprites only — the geometry `draw_bench`'s `sprites`
and `quads` kinds exercise. Circles, polygons, capsules, and text continue through the
existing CPU-expansion path unmodified. A batch (spritebatch-produced group) is only
routed through the new instanced path if every item in it is simple sprite/quad geometry
with no rotation-needing SDF fields in play; otherwise it falls back to the legacy path.
This keeps the prototype's blast radius contained and directly comparable against the
existing `sprites`/`quads`/`mixed` baselines already captured for `spritebatch-perf`.

**Approach.** Use CF's existing public instancing API, unused today:
`cf_mesh_set_instance_buffer` / `cf_mesh_update_instance_data` /
`CF_VertexAttribute::per_instance` (`include/cute_graphics.h:1390-1484`). Set up a small
static unit-quad base mesh (non-instanced, uploaded once), and a compact per-instance
struct (~36 bytes — exact field packing, e.g. how transform/rotation is represented, is an
implementation detail for the plan/TDD phase, not fixed here) carrying world-space
position/size/rotation, an atlas UV rect, and color+alpha. One texture atlas per batch is
unchanged from today (spritebatch still owns atlas assignment); instancing only removes the
CPU-side 6x-per-quad vertex duplication and per-vertex transform/SDF math, not the
one-texture-per-batch constraint.

**Shader work.** Author a new vertex shader (via CF's shader compiler, `tools/cute_shader.cpp`,
following the pattern in `src/data/builtin_shaders_bytecode.h`) that does real GPU-side
work: expand `gl_VertexID` (0-3) against the per-instance transform into a world-space
corner, apply the camera/projection to produce the homogeneous position, and compute UV
from the instance's atlas rect. The existing fragment shader is expected to be reusable
unchanged for the simple sprite/quad case (confirming its exact varying requirements is
part of implementation, not this design).

**Fallback if scope proves too large mid-session:** drop to Approach C (vertex-format diet
— shrink `CF_Vertex` for the sprite/quad path by dropping the unused 64-byte `shape[8]`,
no shader or instancing-API changes) and report that instead, with a note that it validates
a smaller, lower-risk slice of the same hypothesis (~3.8x rather than ~6x).

**Success criteria:** working instanced draw path for `sprites`/`quads` `draw_bench` kinds,
correctness verified visually (no misplaced/misscaled quads) and via existing tests where
applicable, before/after `draw_bench` comparison at 100k for `sprites`, `quads`, and `mixed`
kinds against the `spritebatch-perf` baseline.

## Item 4: frustum/viewport culling

**Scope.** All primitive types funnel through the same choke point before entering a
batch: the `DRAW_PUSH_ITEM(s)` macro (`src/internal/cute_draw_internal.h:99-100`:
`#define DRAW_PUSH_ITEM(s) s_draw->cmds.last().items.add(s)`), called from every
`cf_draw_*` implementation (sprite, quad, box, circle, capsule, polygon, text-per-glyph,
etc. — roughly 20 call sites, e.g. `src/cute_draw.cpp:611,834,1030` for the sprite family
alone) once that call's `BatchGeometry` is fully populated. By that point, CF has *already*
computed the item's post camera-and-projection (clip-space) corners — `m = s_draw->mvp;
CF_MUL_M32_V2(s.geom.shape[0], m, quad[0]); ...` (`src/cute_draw.cpp:602-606`) for
sprite/quad-like geometry, with parallel `box`/`boxH` fields (`BatchGeometry`,
`src/internal/cute_draw_internal.h:32-54`) for box-type shapes. This means culling doesn't
need any new transform math — only a bounds check on data CF already has in hand at that
exact point.

**Approach (recommended): fold the visibility test into the `DRAW_PUSH_ITEM` macro
itself.** Compute the clip-space AABB of the item's already-transformed corners (switching
on `geom.type` the same way the existing vertex-expansion loop does,
`src/cute_draw.cpp:~300-360`, to pick `shape[0..3]`, `boxH[0..3]`, or `shape[0..n]` for
polygons), and skip the `items.add(s)` entirely if that AABB doesn't overlap the canonical
clip-space viewport (`[-1,1]×[-1,1]`, inflated by a small margin for stroke width/AA bleed —
exact margin handling is an implementation detail for the plan, not fixed here). Because
`DRAW_PUSH_ITEM` is a single macro used at every call site, this is a one-place change that
benefits all ~20 draw entry points automatically, with no per-call-site edits needed.
Rotated/zoomed cameras are handled correctly for free, since the corners being tested are
already post-`mvp` (camera × projection combined, `src/cute_draw.cpp:3765`) — no separate
world-space-vs-rotated-viewport (OBB) test is needed.

**Alternatives considered:**
- **Per-call-site early-out**, testing before any local geometry/transform math runs (top
  of each `cf_draw_*` function). Would save slightly more CPU per culled item (skips the
  quad construction + matrix multiply, not just the batch push), but requires touching
  every one of the ~20 entry points individually, and needs its own transform or a
  conservative world-space-vs-camera-AABB test to work correctly under rotation — since the
  clip-space corners this approach reuses for free don't exist yet at that point. Rejected:
  meaningfully more invasive for a marginal additional saving.
- **Spatial index (quadtree/grid)**, populated once by the game and queried per frame for
  only the visible subset. Strictly more powerful for huge static worlds — avoids the O(n)
  per-item test entirely, rather than just skipping the expensive part of processing for
  culled items — but requires new persistent API surface and a retained-mode usage pattern
  that CF's immediate-mode `cf_draw_*` philosophy doesn't have today. Out of scope for this
  item; worth revisiting as a future follow-up if per-item AABB testing itself becomes the
  bottleneck at very high world-object counts.

**Correctness risk.** The clip-space margin for stroke thickness/antialiasing bleed needs
to be right, or shapes whose fill is just off-screen but whose stroked/AA edge pokes into
view will visibly pop in/out at the viewport boundary. Verify with a sample that pans/zooms/
rotates the camera over a dense field of shapes near the viewport edge, watching for
pop-in/pop-out, in addition to whatever automated coverage the implementation plan adds.

**Success criteria:** culling implemented behind the single `DRAW_PUSH_ITEM` choke point; a
new `draw_bench` kind (working name `scattered`) that draws N items across a world several
times larger than the viewport, with only a small fraction actually visible; before/after
comparison at a few visible fractions (e.g. 100%, 10%, 1%) to show the win scales with
off-screen fraction rather than being a fixed constant factor; a visual correctness check
sweeping camera translate/rotate/zoom with no visible pop-in/pop-out at the viewport edge.

## Benchmarking methodology (all items)

Per existing project convention (see `spritebatch-perf` memory): frame times on this Mac
are bimodal (~3x swings from P-core/E-core/compositor throttling; occasional vsync-lock to
~16.6ms despite `cf_app_set_vsync(false)`). Always interleave A/B binaries round-robin and
compare min-of-rounds p50, or rely on phase-timer/microbench medians where available (item
2's instrumentation). Reuse and extend `samples/draw_bench.c` — its existing kinds and N
tiers for items 2/3, plus the new `scattered` kind for item 4 — rather than introducing a
separate benchmark harness.

## Open questions / risks

- Item 3's biggest unknown is shader-authoring turnaround with `tools/cute_shader.cpp` —
  if that tool's workflow is heavier than expected, the fallback (Approach C) absorbs that
  risk without blocking a result.
- Item 4's biggest unknown is the clip-space margin for stroke/AA bleed at the viewport
  edge — too tight and visible content pops in/out at the boundary, too loose and some of
  the culling win is given back. This needs empirical tuning against the visual-correctness
  check described in §Item 4, not just the benchmark numbers.
- Items 2, 3, and 4 are independent by construction (separate worktrees — item 2 off
  `master`, items 3/4 off `spritebatch-perf`'s tip), so a slip in one doesn't block
  delivering results for the others, at the cost of a likely rebase for item 2 wherever it
  and `spritebatch-perf` eventually meet (see §Workstream setup).
