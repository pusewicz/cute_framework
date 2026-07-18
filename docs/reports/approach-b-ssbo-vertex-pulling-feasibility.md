# Approach B Feasibility: Full SSBO Vertex-Pulling for All Shape Types

**Date:** 2026-07-18
**Branch:** `master` @ `bb3a0aee`
**Scope:** Feasibility assessment of "Approach B" from
[`docs/superpowers/specs/2026-07-18-draw-batcher-churn-and-instancing-design.md`](../superpowers/specs/2026-07-18-draw-batcher-churn-and-instancing-design.md) —
using GPU storage buffers to vertex-pull per-primitive data for *every* CF shape type
(sprites, quads, circles, polygons, capsules, text), as an alternative to the scoped
instanced-quad prototype (Approach A) that design chose instead.

---

## Executive summary

**Approach B is not viable as scoped, and shouldn't be attempted before Approach A.**

CF's `CF_StorageBuffer` API exists and works for **compute** shaders, but there is
**no binding path to attach a storage buffer to a graphics (vertex/fragment) draw call at
all** — no material API, no backend bind call, no field on `CF_MaterialState` to hold one.
The `graphics_readable` flag that would signal "usable from vertex/fragment stage" exists,
defaults to `false`, and is set to `true` by **zero call sites** anywhere in the repo.
Building Approach B means building this binding layer from scratch on top of an SDL_GPU
capability (`SDL_BindGPUVertexStorageBuffers`/`SDL_BindGPUFragmentStorageBuffers`) that CF's
backend never calls today.

Worse, CF's GLES3 backend — which **is** the Emscripten/WebGL2 web target
(`CMakeLists.txt:234-249`) — implements storage buffers as pure no-op stubs that return an
invalid `{0}` handle. Approach B's core premise ("uniformly, for all shape types") is
incompatible with keeping web support unless a full parallel CPU path is maintained anyway,
which defeats the point of the rewrite.

| Question | Answer |
|---|---|
| Can a storage buffer be read from a vertex/fragment shader today? | **No runtime path exists** — buffer object exists, bind call doesn't |
| Does the SDL_GPU backend support it structurally? | Buffer create/update: yes. Bind-to-draw: no, and shader-object creation already reflects storage-buffer counts, so this is additive, not blocked |
| Does the GLES3/web backend support it? | **No — explicit no-op stubs**, hard dead end |
| Is the existing 8-point polygon SDF shader reusable as-is? | No — fixed `vec2[8]` array, must be rewritten for variable-length data regardless of vertex-pulling |
| Is per-batch storage-buffer update cheaper than today's vertex-buffer update? | **No — architecturally identical cost** (same map/copy/unmap + copy-pass + render-pass break) |
| Is any part of Approach B's plumbing already mature? | Yes — GPU instancing (the non-SSBO half) is proven cross-backend, used in 2 samples. This is what Approach A already uses. |

**Recommendation:** stay with Approach A (scoped instanced-quad prototype). Approach B's
only real advantage over A — handling every shape type through one uniform path instead of
an instanced/legacy split — is not worth the cost: a from-scratch graphics-storage-buffer
binding layer, a GLES3/web dead end that forces a parallel path anyway, and a fragment
shader rewrite for variable-length polygons that A doesn't need. Approach B is worth
revisiting only if CF ever drops GLES3/WebGL2 as a target, or after a maintainer decides
the graphics-storage-buffer binding layer is worth building for other reasons (e.g.
GPU-driven culling, bindless textures) — at which point vertex-pulling could ride on top of
it more cheaply than building it just for this.

---

## 1. What Approach B would need

Approach B (as scoped in the design doc's rejected-alternative section) proposed: a tiny
static index/vertex buffer, drawn with GPU instancing, where the vertex (and/or fragment)
shader reads full per-primitive data — including variable-length polygon point arrays —
from one or more `CF_StorageBuffer`s indexed by `gl_InstanceID`/`gl_VertexID`. This avoids
the instanced/legacy split Approach A requires, because circles, polygons, capsules, and
text would all flow through the same storage-buffer-driven path as simple quads.

That requires, at minimum:
1. A public API to bind a storage buffer to a material for vertex and/or fragment reads
   (something like `cf_material_set_storage_buffer_vs`/`_fs`).
2. Backend implementation of that bind, on every backend CF ships.
3. A rewrite of the built-in draw fragment shader's polygon SDF math, which is currently
   hard-capped at 8 points.
4. A per-primitive record layout compact enough to be worth reading per-vertex, plus a
   side buffer (or offset scheme) for variable-length polygon point lists.

None of these four exist today. The rest of this report is the evidence for each gap.

---

## 2. `CF_StorageBuffer` exists, but only reaches compute shaders

The public API (`include/cute_graphics.h:159-1031`) is real and documented:

```c
typedef struct CF_StorageBuffer { uint64_t id; } CF_StorageBuffer;

typedef struct CF_StorageBufferParams
{
	int size;
	bool compute_readable;   // default true
	bool compute_writable;   // default false
	bool graphics_readable;  // default false -- exists, but nobody sets it
} CF_StorageBufferParams;

CF_API CF_StorageBuffer CF_CALL cf_make_storage_buffer(CF_StorageBufferParams params);
CF_API void CF_CALL cf_update_storage_buffer(CF_StorageBuffer buffer, const void* data, int size);
CF_API void CF_CALL cf_destroy_storage_buffer(CF_StorageBuffer buffer);
```

The doc comment on `CF_StorageBuffer` itself already flags the gap this report confirms
structurally: *"Storage buffers are GPU-accessible buffers used with compute and graphics
shaders. They are only available on SDL_GPU backends (not GLES3)."* — graphics-shader
support is aspirational in the doc comment; the code doesn't back it yet.

The only place a `CF_StorageBuffer` array can actually be attached to a draw is
`CF_ComputeDispatch` (`include/cute_graphics.h:1036-1101`), which is exclusively for
`cf_dispatch_compute`:

```c
typedef struct CF_ComputeDispatch
{
	CF_StorageBuffer* rw_buffers;   // bound at compute pass creation
	int rw_buffer_count;
	CF_StorageBuffer* ro_buffers;   // bound after pipeline bind
	int ro_buffer_count;
	...
} CF_ComputeDispatch;
```

`CF_MaterialState` — the struct that actually holds what gets bound to a *graphics* draw
call — has no storage-buffer field at all (`src/internal/cute_graphics_internal.h:139-143`):

```c
struct CF_MaterialState
{
	Cute::Array<CF_Uniform> uniforms;
	Cute::Array<CF_MaterialTex> textures;
};
```

**Usage in the repo confirms this is unexercised, not just unbound.** The API was added
~5 months before this commit (`ba6b599`, "Compute shader support", 2026-02-06). Its only
real consumer anywhere in `src/`, `samples/`, or `test/` is `samples/hrc.c`, and every use
there is compute-only (`cf_dispatch_compute`). `graphics_readable` is never set to `true`
anywhere. No tests reference storage buffers at all.

---

## 3. Backend support: SDL_GPU is additive work, GLES3/web is a dead end

### SDL_GPU (Metal/Vulkan/D3D12) — buffer plumbing exists, bind call doesn't

SDL3's GPU API already exposes `SDL_BindGPUVertexStorageBuffers` and
`SDL_BindGPUFragmentStorageBuffers`. CF's backend never calls either — the only
`SDL_BindGPU*StorageBuffers` call in `src/cute_graphics_sdlgpu.cpp` is
`SDL_BindGPUComputeStorageBuffers`, inside the compute dispatch path. The function that
binds everything for a *draw* call, `cf_sdlgpu_apply_shader`
(`src/cute_graphics_sdlgpu.cpp:1682-1829`), binds vertex/index buffers, VS/FS samplers, and
pushes uniform blocks — no storage-buffer bind exists in it.

This is genuinely additive work rather than blocked work: shader **creation/reflection**
already threads `num_storage_buffers` through for regular (non-compute) shaders
(`src/cute_graphics_sdlgpu.cpp:663-681`), and `docs/topics/shader_compilation.md:149-167`
documents that graphics vertex/fragment shaders may declare storage buffers — the toolchain
anticipated this, but no runtime binding path consumes it. Every built-in draw shader today
declares `.num_storage_buffers = 0` (`src/data/builtin_shaders_bytecode.h`, all shader-info
blocks).

There's also a live defect that would bite Approach B specifically:
`cf_sdlgpu_update_storage_buffer`'s resize path hardcodes
`SDL_GPU_BUFFERUSAGE_COMPUTE_STORAGE_READ` instead of reusing the caller's original usage
flags (`src/cute_graphics_sdlgpu.cpp:2046-2065`, comment: `// For simplicity, use READ
since we're uploading.`). `CF_StorageBufferInternal` doesn't even store the original params
to recover from. A per-primitive storage buffer that grows across frames as scene
complexity changes — exactly Approach B's use case — would silently lose
`GRAPHICS_STORAGE_READ` on its first regrow.

### GLES3/WebGL2 — explicit no-op stubs, and this is CF's actual web backend

`src/cute_graphics_gles.cpp:1758-1797`, under the header `// Compute stubs (not supported
on GLES3).`:

```c
CF_StorageBuffer cf_gles_make_storage_buffer(CF_StorageBufferParams params)
{
	CF_UNUSED(params);
	CF_StorageBuffer result = { 0 };
	return result;
}

void cf_gles_update_storage_buffer(CF_StorageBuffer buffer, const void* data, int size)
{
	CF_UNUSED(buffer); CF_UNUSED(data); CF_UNUSED(size);
}
```

`cf_make_storage_buffer` returns a zeroed, invalid handle rather than failing loudly — code
that "worked" against the SDL_GPU backend would silently do nothing (or crash on first
dereference) on GLES3. And GLES3 isn't a minor fallback: `CMakeLists.txt:234-249` builds
this exact file for Emscripten/Web, CF's only web target (`-sUSE_WEBGL2=1
-sMIN_WEBGL_VERSION=2 -sMAX_WEBGL_VERSION=2`), and the dispatch-shim macros in
`src/cute_graphics.cpp:630-745` hard-route every storage-buffer call to `cf_gles_*` when
building for Emscripten — there's no alternate backend compiled in for web at all.

**This means Approach B, if built "uniformly for all shape types" as scoped, either drops
web support or requires maintaining the existing CPU-expansion path anyway as a GLES3
fallback — which is most of Approach B's implementation cost for none of its "one uniform
path" benefit.**

---

## 4. The fragment shader's SDF math needs a rewrite regardless of vertex-pulling

The built-in draw fragment shader (`src/data/builtin_shaders_bytecode.h:349-703`, embedded
GLSL source) branches on a `type` varying (0-6: sprite, text, box, segment, triangle,
triangle-SDF, polygon) and does one closed-form SDF distance evaluation per shape kind.
Circles and capsules both render as `VA_TYPE_SEGMENT` (`src/cute_draw.cpp:302,321`), not
their own branch.

The polygon path is hard-capped:

```glsl
vec2 pts[8];
...
float distance_polygon(vec2 p, vec2[8] v, int N) { ... }
```

`CF_Vertex.shape[8]` (`include/cute_draw.h:606`) matches this cap exactly — 8
`CF_V2`s, fed via four `vec4` varyings (`v_ab`/`v_cd`/`v_ef`/`v_gh`). This fixed-size array
is baked into the shader regardless of how the data arrives (CPU-expanded vertices today,
or a storage buffer under Approach B). Supporting arbitrary polygon point counts — which is
part of Approach B's premise ("all shape types," including arbitrarily complex polygons) —
requires rewriting `distance_polygon` to read a dynamically-bounded range from a buffer.
This work is real either way; it isn't a cost specific to vertex-pulling, but it is a cost
Approach A doesn't have to pay at all, since Approach A only targets plain quads/sprites
and leaves polygons on the existing path untouched.

All shape reconstruction (box orientation via `mat2` rotation, segment endpoints, triangle
verts, polygon points) currently arrives at the fragment shader **already in world space**
— the vertex shader is a pure passthrough (`src/data/builtin_shaders_bytecode.h:1-47`;
confirmed independently in the design doc's own investigation). A vertex-pulling rewrite
would need its vertex stage to reconstruct all of this — corner expansion, world-space
transform, per-shape-type field layout — from a compact SSBO record, which is real
GPU-side work that doesn't exist in any form today, for any shape type.

---

## 5. No update-cost win over Approach A at batch granularity

`s_draw_report`'s only real per-batch GPU cost today is `cf_mesh_update_vertex_data`
(`src/cute_draw.cpp:369`); everything else in that function
(`cf_material_set_texture_fs`, `cf_material_set_uniform_fs` ×4) is pure CPU bookkeeping,
and `cf_apply_shader`'s GPU-side binds (samplers, vertex/index buffers, push-constant-style
uniform upload via `SDL_PushGPUVertexUniformData`) don't break the render pass.

`cf_sdlgpu_mesh_update_vertex_data` → `s_update_buffer`
(`src/cute_graphics_sdlgpu.cpp:1350-1395`) and `cf_sdlgpu_update_storage_buffer`
(`src/cute_graphics_sdlgpu.cpp:2041-2085`) follow the **identical shape**: both call
`s_end_active_pass()` (ending the current render pass), map/copy/unmap a transfer buffer,
then `SDL_BeginGPUCopyPass`/`SDL_UploadToGPUBuffer`/`SDL_EndGPUCopyPass`. If Approach B
updated its storage buffer once per batch — the same cadence CF already pays for vertex
uploads today — the incremental GPU cost is a wash, not a win. Approach B's appeal was
never "cheaper uploads"; it was "less CPU-side data to build per shape." That benefit is
real but is exactly what Approach A already captures for the quad/sprite case, at a much
smaller implementation cost.

If Approach B instead needed per-individual-draw-call updates (e.g. because different shape
types can't share one storage-buffer record layout and end up as separate draws within what
is today a single texture-atlas batch), it would multiply render-pass breaks past current
behavior — a real regression risk the design doesn't currently have an answer for.

---

## 6. What *is* solid: instancing, which Approach A already uses

GPU instancing — the non-SSBO half of what Approach B proposed — is mature and
cross-backend today. `cf_mesh_set_instance_buffer`/`cf_mesh_update_instance_data`
(`include/cute_graphics.h:1390-1484`) are implemented in both
`src/cute_graphics_sdlgpu.cpp` and `src/cute_graphics_gles.cpp`, and used in two existing
samples (`samples/basic_instancing.c`, `samples/galaxy.c`). `cf_sdlgpu_apply_shader`/
`cf_sdlgpu_draw_elements` already branch on `mesh->instances.buffer`
(`src/cute_graphics_sdlgpu.cpp:1753-1765`, `:1831-1844`). This is exactly the API Approach A
is scoped to use, and it's proven, unlike the storage-buffer graphics-bind path Approach B
depends on.

---

## 7. Conclusion

Approach B is the architecturally "cleaner" long-term idea — one code path for every shape
type instead of an instanced/legacy split — but it's cleaner on paper only. In this
codebase, today:

- The binding layer it needs doesn't exist and must be built from scratch on both the
  public API and SDL_GPU backend sides.
- The web/GLES3 backend makes storage buffers a hard no-op, so "uniformly for all shape
  types" is false unless web support is dropped or a parallel CPU path is kept anyway.
- Its one clear GPU-side upload-cost advantage over the current vertex-buffer approach
  doesn't materialize at batch granularity — both are the same map/copy/render-pass-break
  operation.
- It requires a fragment shader rewrite (variable-length polygon SDF) that Approach A
  sidesteps entirely by scoping to quads/sprites.

None of this makes Approach B wrong forever — if CF ever builds a general graphics-storage-buffer
binding layer for other reasons (GPU-driven culling, bindless textures, compute-driven
particle systems), vertex-pulling for shapes could ride on top of it cheaply. But building
that layer *just* to unify shape rendering, while carrying a GLES3 dead end, is not a good
trade against Approach A's much smaller, already-buildable scope. The design doc's choice
of Approach A stands.

---

## Appendix A — Source anchors

```
include/cute_graphics.h:159-1101        — CF_StorageBuffer, CF_StorageBufferParams, CF_ComputeDispatch
src/internal/cute_graphics_internal.h:139-143 — CF_MaterialState (no storage-buffer field)
src/cute_graphics.cpp:630-745           — dispatch shims, Emscripten routing
src/cute_graphics_sdlgpu.cpp:1350-1395  — s_update_buffer (vertex buffer upload path)
src/cute_graphics_sdlgpu.cpp:1682-1829  — cf_sdlgpu_apply_shader (draw-call binding, no SSBO bind)
src/cute_graphics_sdlgpu.cpp:1907-2085  — CF_StorageBufferInternal, storage buffer create/update
src/cute_graphics_sdlgpu.cpp:2177       — only SDL_BindGPU*StorageBuffers call in the file (compute-only)
src/cute_graphics_gles.cpp:1758-1797    — storage buffer no-op stubs ("not supported on GLES3")
src/cute_draw.cpp:53-59                 — VA_TYPE_* shape-type constants
src/cute_draw.cpp:123-412               — s_draw_report (today's per-batch cost profile)
src/cute_draw.cpp:302,321               — circle/capsule emit VA_TYPE_SEGMENT
src/cute_draw.cpp:468-479               — vertex attribute layout / CF_Vertex field mapping
src/data/builtin_shaders_bytecode.h     — built-in draw VS (passthrough) + FS (SDF branch), embedded GLSL source in comments
include/cute_draw.h:594-637             — CF_Vertex struct
docs/topics/shader_compilation.md:149-167 — documented (unused) storage-buffer set/binding convention
CMakeLists.txt:111,234-249              — GLES backend is CF's Emscripten/WebGL2 web target
```

---

*End of report.*
