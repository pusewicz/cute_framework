# SDL3 GPU Integration in Cute Framework — Architecture Review

This report describes how Cute Framework (CF) integrates SDL3's GPU API, documents bugs found
during review (with suggested fixes), and lists simplification and performance opportunities.

Scope of review:

- `src/cute_graphics_sdlgpu.cpp` (the SDL_GPU backend, ~2,200 lines)
- `src/cute_graphics.cpp` (backend-agnostic layer + dispatch shims)
- `src/internal/cute_graphics_internal.h`
- Call sites in `src/cute_app.cpp`, `src/cute_imgui.cpp`, and `src/cute_draw.cpp`

---

## 1. Architecture overview

### 1.1 Backend dispatch

CF supports two graphics backends: **SDL3 GPU** (Vulkan / Metal / D3D11 / D3D12 / "private")
and **GLES3** (used on Emscripten, and as an Android fallback). Public `cf_*` graphics
functions are routed through macro-generated shims in `cute_graphics.cpp:646-750`:

```c
#define CF_DISPATCH_SHIM(RETURN_TYPE, OP, ARGUMENTS, ...) \
    RETURN_TYPE cf_##OP ARGUMENTS { \
        if (app->gfx_backend_type == CF_BACKEND_TYPE_GLES3) return cf_gles_##OP(__VA_ARGS__); \
        else return cf_sdlgpu_##OP(__VA_ARGS__); \
    }
```

On Emscripten only the GLES path is compiled; otherwise both backends are compiled and the
choice is made at runtime per `app->gfx_backend_type`.

### 1.2 Initialization and frame lifecycle

- **Init** (`cf_make_app` → `cf_sdlgpu_init`, `cute_graphics_sdlgpu.cpp:744`): initializes
  SDL_shadercross, creates the `SDL_GPUDevice` with `SDL_ShaderCross_GetSPIRVShaderFormats()`,
  optionally forcing a driver (`direct3d11`/`direct3d12`/`metal`/`vulkan`). On Windows debug
  builds it patches the D3D12 info queue to suppress a clear-value warning by reading SDL's
  *private* struct layout via build-time-generated offsets (`:698-742`).
- **Attach** (`cf_sdlgpu_attach`, `:792`): claims the window, sets present mode
  (default **IMMEDIATE**, i.e. no vsync), and acquires the first command buffer.
- **Frame**: `cf_sdlgpu_begin_frame` acquires one `SDL_GPUCommandBuffer` per frame
  (`g_ctx.cmd`). All rendering for the frame is recorded into it. `cf_sdlgpu_blit_canvas`
  lazily acquires the swapchain texture (`SDL_WaitAndAcquireGPUSwapchainTexture`) and blits
  the app's offscreen canvas onto it. Dear ImGui then renders its own pass directly into the
  swapchain (`cute_imgui.cpp:52-82`). `cf_sdlgpu_end_frame` submits the command buffer.
- **Global state** lives in a single static `g_ctx` (`:154-166`): device, current command
  buffer, window, swapchain texture, current canvas, the active render pass, and a
  draw-system sampler override.

### 1.3 Render pass management

Render passes are begun *lazily*: `cf_apply_canvas` only records the target canvas and a
pending-clear flag; the actual `SDL_BeginGPURenderPass` happens inside `cf_sdlgpu_apply_shader`
(`:1712-1749`), using `LOADOP_CLEAR` + `cycle=true` when a clear is pending, else
`LOADOP_LOAD`. The pass stays open across consecutive draws to the same canvas and is ended
(`s_end_active_pass`, `:168`) whenever something incompatible happens: switching canvases,
requesting a clear, any texture/buffer upload (copy passes can't nest inside render passes),
compute dispatch, GPU sync, or frame end. MSAA canvases resolve at pass end via
`STOREOP_RESOLVE_AND_STORE` into a separate resolve texture.

### 1.4 Shaders, reflection, and pipelines

- CF shaders are compiled (offline or at runtime via `cute_shader`/glslang) to **SPIR-V plus a
  reflection blob** (`CF_ShaderInfo`): vertex inputs, uniform blocks/members, sampler names and
  binding slots, resource counts.
- If the device natively consumes SPIR-V (Vulkan), `SDL_CreateGPUShader` is used directly;
  otherwise SDL_shadercross cross-compiles SPIR-V to the device format (`:674-693`).
- **Pipelines** are created on demand in `cf_sdlgpu_apply_shader` and cached per shader in a
  linear array keyed by a content-based `CF_PipelineKey` (canvas color/depth formats, sample
  count, full `CF_RenderState`, and the matched vertex layout), compared with `memcmp`
  (`:1691-1706`).
- **Uniforms** are name-matched from the material's uniform list into shader uniform blocks
  built in a scratch arena, then pushed with `SDL_PushGPU*UniformData` (`s_copy_uniforms`,
  `:1480`). **Textures** are name-matched from the material to reflection sampler names and
  bound with `SDL_BindGPU*Samplers`.
- **Meshes** hold vertex/index/instance `SDL_GPUBuffer`s, each paired with a persistent upload
  transfer buffer; updates go map → memcpy → copy pass, growing buffers 2× on overflow
  (`s_update_buffer`, `:1350`).
- **Compute** has a parallel path: `SDL_GPUComputePipeline`, storage buffers, and
  `cf_sdlgpu_dispatch_compute` (`:2096`) which binds read/write storage resources, name-matched
  samplers and uniforms, then dispatches.

---

## 2. Bugs and potential fixes

Ordered roughly by severity.

### 2.1 `cf_canvas_readback` reads stale data — download races ahead of unsubmitted rendering

`cf_sdlgpu_canvas_readback` (`:1152-1216`) records the GPU→CPU copy into a **freshly acquired
command buffer** and submits it immediately, while the frame's rendering commands still sit
*unsubmitted* in `g_ctx.cmd` (nothing submits `g_ctx.cmd` until `end_frame`). SDL GPU executes
command buffers in **submission order**, so the download executes before the rendering that
was just recorded, and the readback returns the canvas contents from the *previous* submit.
The documented usage in `cute_graphics.h` (`cf_render_to(canvas, true)` immediately followed by
`cf_canvas_readback(canvas)`) hits exactly this path, since `cf_render_to` records but does not
submit.

**Fix**: submit the pending work first, then record the download:

```c
s_end_active_pass();
if (g_ctx.cmd) {
    SDL_SubmitGPUCommandBuffer(g_ctx.cmd);
    g_ctx.cmd = SDL_AcquireGPUCommandBuffer(g_ctx.device);
}
// ...then acquire the readback cmd, record the copy pass, submit + acquire fence as today.
```

(Alternatively record the download into `g_ctx.cmd` itself and submit it with
`SDL_SubmitGPUCommandBufferAndAcquireFence`, then reacquire.)

### 2.2 Storage buffer auto-resize silently drops usage flags

`cf_sdlgpu_update_storage_buffer` (`:2047-2065`) recreates a grown buffer with only
`SDL_GPU_BUFFERUSAGE_COMPUTE_STORAGE_READ` — the code comment even acknowledges it. A buffer
created `compute_writable` or `graphics_readable` loses those capabilities the first time it
grows, causing SDL validation errors / broken rendering on the next bind.

**Fix**: store the original `SDL_GPUBufferUsageFlags` in `CF_StorageBufferInternal` at creation
and reuse them on resize.

### 2.3 Sampler binding slots from reflection are collected but never used

`s_load_shader_bytecode` carefully records `image_binding_slots` per stage (`:603`, `:608`; same
for compute at `:1924`), but the bind loops in `cf_sdlgpu_apply_shader` (`:1775-1819`) and
`cf_sdlgpu_dispatch_compute` (`:2142-2158`) fill the binding array by *reflection order index
`j`* and always bind starting at slot 0. This is correct only while a shader's sampler bindings
are contiguous from 0 **and** reflection order matches slot order. A shader with e.g. samplers
at slots {0, 2} or reflection order ≠ slot order gets textures bound to the wrong slots.

A second issue in the same loops: if a material fails to provide a texture for every shader
sampler, `found_*_count < sampler_count` and the only guard is a debug-only `CF_ASSERT`. In
release builds `SDL_BindGPU*Samplers` is called with a **partially uninitialized stack array**
(entries for unmatched names are never written), binding garbage pointers.

**Fix**: index the binding array with `vs_image_binding_slots[j]` instead of `j` (sizing the
array by max slot + 1), and zero-init the array + bail out (or bind a default 1×1 texture) when
a name can't be matched, rather than proceeding with uninitialized entries.

### 2.4 `cf_clear_canvas` ignores the user's clear depth/stencil values

`cf_sdlgpu_clear_canvas` (`:1101-1129`) hardcodes `clear_depth = 1.0f` and `clear_stencil = 0`,
while the lazy clear path in `apply_shader` (`:1734-1735`) honors `app->clear_depth` /
`app->clear_stencil` set via `cf_clear_depth_stencil`. The two clear paths disagree.

**Fix**: use `app->clear_depth` / `app->clear_stencil` in `cf_sdlgpu_clear_canvas`.

### 2.5 `cf_clear_canvas` does not clear the resolve texture of an MSAA canvas

The explicit clear pass targets only `canvas->texture` (the MSAA target) with
`STOREOP_STORE` and no resolve. `cf_canvas_get_target` and `cf_sdlgpu_blit_canvas` read the
*resolve* texture for MSAA canvases, so a clear followed by a blit (with no draws in between)
shows stale resolve contents.

**Fix**: for MSAA canvases use `STOREOP_RESOLVE` with `resolve_texture` set in the clear pass —
or implement `clear_canvas` as a deferred clear (set `canvas->clear = true`), unifying it with
the lazy-clear mechanism and eliminating the extra pass entirely.

### 2.6 Streaming texture updates can overflow the transfer buffer

For `params.stream` textures, `s_make_texture` creates a persistent transfer buffer sized
exactly `texel_size * w * h` (`:566-575`). `cf_sdlgpu_texture_update` (`:934-967`) then maps it
and `memcpy`s the caller-provided `size` with **no bounds check** — a `size` larger than the
texture overflows driver memory. Conversely, a `size` *smaller* than `w*h*texel_size` still
issues a full-region `SDL_UploadToGPUTexture`, making the GPU read past the end of the copied
data (validation error or garbage texels). `texture_update_mip` has the same pattern.

**Fix**: clamp/validate `size` against the destination region
(`CF_ASSERT(size >= region_bytes)` plus a release-mode early-out), and derive the upload region
from `size` where sensible.

### 2.7 `cf_apply_viewport` / `cf_apply_scissor` require a pass that may not exist, and state is lost on pass splits

These functions (`:1428-1452`) assert `g_ctx.canvas->pass` and then use it. The pass is only
created inside `cf_apply_shader`, so the natural-looking call order
`cf_apply_canvas → cf_apply_viewport → cf_apply_shader` asserts in debug and calls SDL with a
NULL pass in release. CF's internal draw layer happens to call them after `cf_apply_shader`
(`cute_draw.cpp:395-409`), so only direct users of the low-level API hit this. Additionally,
since SDL resets viewport/scissor at each pass boundary and CF splits passes implicitly (any
texture/buffer upload, canvas switch, or compute dispatch), user-set viewport/scissor silently
reverts mid-frame.

**Fix**: latch viewport/scissor/stencil-ref/blend-constants in `g_ctx`, apply them (if set)
whenever a render pass begins, and make the `cf_apply_*` functions store state instead of
requiring a live pass.

### 2.8 Uniform-block index from reflection is not bounds-checked

`s_load_shader_bytecode` (`:620-626`) writes `vs_block_sizes[block_index]` where arrays are
sized `CF_MAX_UNIFORM_BLOCK_COUNT` (4, `cute_graphics_internal.h:18`) using an index read
straight from the reflection blob, with no bounds check. A shader with >4 uniform blocks (or a
corrupted/hand-authored bytecode blob) writes out of bounds. Same pattern in the compute path
(`:1930-1948`).

**Fix**: `CF_ASSERT(block_index < CF_MAX_UNIFORM_BLOCK_COUNT)` plus a release-mode skip.

### 2.9 `CF_MEMSET` over freshly constructed C++ objects

`cf_sdlgpu_make_shader_from_bytecode` (`:1032-1033`) and
`cf_sdlgpu_make_compute_shader_from_bytecode` (`:1916-1917`) do `CF_NEW(T)` (which runs
constructors, including the `Cute::Array` members) and then `CF_MEMSET(ptr, 0, sizeof(*ptr))`.
Today this is benign because `Array`'s default state is all-zero, but it is formally UB and will
break the moment `Array` (or any member) gains a non-trivial default state. The same conceptual
hazard exists in `cf_sdlgpu_destroy_shader_internal`, which manually calls the destructor then
`CF_FREE` — correct, but tightly coupled to allocator details.

**Fix**: delete the `CF_MEMSET` lines (members already have default initializers / default
constructors).

### 2.10 Missed submit when a frame is abandoned

`cf_sdlgpu_begin_frame` (`:836-840`) unconditionally overwrites `g_ctx.cmd` with a newly
acquired command buffer. If the previous frame never reached `end_frame` (app skipped
`cf_app_draw_onto_screen`), the old command buffer is leaked — SDL requires every acquired
command buffer be submitted or canceled.

**Fix**: `if (g_ctx.cmd) SDL_SubmitGPUCommandBuffer(g_ctx.cmd);` at the top of `begin_frame`.

### 2.11 Unchecked SDL return values

Several calls ignore failures that are plausible at runtime:

- `SDL_ClaimWindowForGPUDevice` in `cf_sdlgpu_attach` (`:794`) — on failure everything after
  is undefined.
- `SDL_MapGPUTransferBuffer` results are used without NULL checks in every upload path
  (`:949`, `:986`, `:1377`, `:2068`) — a failed map means `memcpy` to NULL.
- `SDL_CreateGPUBuffer` / `SDL_CreateGPUTransferBuffer` in mesh/storage creation.
- Most creation failures are `CF_ASSERT`-only, which compiles out in release.

**Fix**: check-and-propagate (return zero handles / `CF_Result`), at minimum guard the maps.

### 2.12 Minor / latent items

- **Pipeline-key padding**: `s_make_pipeline_key` copies the whole `CF_RenderState` into the
  key and compares with `memcmp` (`:1660`, `:1695`). If a user builds a `CF_RenderState` by
  hand (not from `cf_render_state_defaults()`, which zero-inits), padding garbage makes
  logically identical states miss the cache and accumulate duplicate pipelines. Zero the state
  inside `cf_material_set_render_state` (copy field-by-field into a zeroed struct) or make the
  key hash field-by-field.
- **`s_query_backend`** (`:472`) matches the driver string `"private"` with a literal
  “Is this the right string??” comment — worth confirming against SDL.
- **Degenerate meshes**: `apply_shader`'s buffer-bind else-branch (`:1762-1764`) binds
  `mesh->vertices.buffer` even when it is NULL (mesh with neither vertex nor instance data);
  a `per_instance` attribute on a mesh without an instance buffer targets vertex-buffer slot 1
  which has no description. Both deserve asserts.
- **D3D12 private-layout hack** (`:698-742`): reading `ID3D12Device*` out of SDL's private
  structs via build-time-generated byte offsets is inherently version-fragile. It is guarded by
  two sanity checks and only runs in Windows debug builds, but any SDL internal reordering
  invalidates the offsets; consider upstreaming a property getter to SDL
  (`SDL_PROP_GPU_DEVICE_D3D12_DEVICE_POINTER` now exists in newer SDL3 releases) and using it
  when available.

---

## 3. Simplification opportunities

1. **Extract a copy-pass helper.** The pattern
   `cmd = g_ctx.cmd ? g_ctx.cmd : SDL_AcquireGPUCommandBuffer(...); begin copy pass; upload;
   end pass; if (!g_ctx.cmd) SDL_SubmitGPUCommandBuffer(cmd);` appears six times
   (`texture_update`, `texture_update_mip`, `generate_mipmaps`, `s_update_buffer`,
   `update_storage_buffer`, `clear_canvas`). A small
   `s_with_copy_pass(callback)`-style helper (or acquire/submit pair) removes ~80 lines and
   makes the submit-ordering rules one place to audit.
2. **Merge `texture_update` and `texture_update_mip`.** `texture_update(t, d, s)` is exactly
   `texture_update_mip(t, d, s, 0)`; the two bodies are near-identical copies.
3. **Unify per-stage shader reflection state.** `CF_ShaderInternal` carries parallel
   `vs_*`/`fs_*` fields (block counts, sizes, members, image names, binding slots) plus
   duplicated `vs_index`/`fs_index` helpers, and `CF_ComputeShaderInternal` duplicates it all a
   third time with `cs_*` names. A single `CF_ShaderStageReflection` struct (used as
   `stages[2]` and reused by compute) would halve `s_load_shader_bytecode`, collapse
   `s_copy_uniforms`'s `vs` boolean branching, and let the two near-identical sampler-binding
   loops in `apply_shader` become one function called twice.
4. **Delete dead code.** `SDL_GPUTextureLocationDefaults` (`:222-230`) is unused;
   `s_texture_supports_format` (`:499-507`) is an unused duplicate of
   `cf_sdlgpu_texture_supports_format`.
5. **Drop cached raw pointers in `CF_CanvasInternal`.** It stores both the `CF_Texture` handles
   and raw `SDL_GPUTexture*`/`SDL_GPUSampler*` copies of the same objects (`:19-36`), and every
   consumer must remember which mirror is authoritative. Deriving the raw pointers on use (one
   cast) removes the duplication and a class of stale-pointer bugs.
6. **Unify the two clear paths.** Implementing `cf_clear_canvas` as “set the deferred-clear
   flag and end the active pass” removes an entire dedicated render pass and fixes bugs 2.4/2.5
   in one motion (with the caveat that a clear with no subsequent draw must still flush before
   readback/blit).
7. **Formatting/consistency nits.** `SDL_GPUSamplerCreateInfoDefaults` uses spaces while the
   file uses tabs (`:191-209`); stray indentation at `:1079` and `:1134-1136`; the
   `CF_DISPATCH_SHIM` special cases for `draw_elements` and `destroy_compute_shader` could be
   folded into the macros.

---

## 4. Performance opportunities

1. **Cache the last-used pipeline / most-recently-used ordering.** `apply_shader` builds a
   ~200-byte key and linearly `memcmp`s it against every cache entry on **every draw call**
   (`:1691-1706`). Consecutive draws overwhelmingly reuse the same pipeline. Storing the last
   `(key, pipeline)` per shader (or moving hits to the array front) makes the common case one
   compare. If user shader counts grow, hash the key (e.g. FNV-1a) and compare hashes first.
2. **Reduce render-pass splitting from mid-frame uploads.** Every
   `cf_mesh_update_*`/`cf_texture_update` call ends the active render pass, opens a copy pass,
   then the next draw re-begins the render pass with `LOADOP_LOAD` (`s_update_buffer:1352`).
   CF's own draw layer updates vertex buffers once per flushed batch, so a frame with N batches
   costs N render passes + N copy passes. On tile-based GPUs (Android is a target via Vulkan)
   each pass split forces a full tile store/load. Options, in increasing effort:
   - Coalesce consecutive copy passes (keep one copy pass open across back-to-back uploads).
   - Stage all draw-layer vertex data into one large per-frame ring buffer, upload once at
     frame start, and draw with `first_vertex`/offset — collapsing to 1 copy pass and far fewer
     render passes per frame.
3. **Skip redundant uniform pushes.** `s_copy_uniforms` re-allocates, zeroes, fills, and pushes
   every matching uniform block on every `apply_shader`, even when the material hasn't changed
   (`:1480-1524`). The material already tracks a `dirty` flag — extend it to uniforms and skip
   pushes when the same (material, shader) pair is re-applied unchanged within a command
   buffer. Also note the O(blocks × uniforms) name-matching loop; fine at current sizes, but a
   per-shader resolved mapping (uniform → block/offset, computed once per material-shader
   pairing) would make it O(uniforms).
4. **Pool transfer buffers.** Non-streaming `texture_update` and every `canvas_readback` create
   and release a transfer buffer per call (`:941-947`, `:1169-1175`). A small size-bucketed
   pool (or reusing one growable upload buffer) avoids per-call driver allocations.
5. **Present-mode default.** `cf_sdlgpu_attach` defaults to `SDL_GPU_PRESENTMODE_IMMEDIATE`
   (`cf_sdlgpu_set_vsync_mailbox(false)`), i.e. vsync off with tearing and uncapped GPU work
   unless the app opts in. Consider defaulting to VSYNC (the universally supported mode) and
   letting games opt into IMMEDIATE/MAILBOX; also note `cf_sdlgpu_set_vsync(false)` and
   `set_vsync_mailbox(false)` are aliases today, which is surprising.
6. **`SDL_WaitAndAcquireGPUSwapchainTexture` late in the frame.** The blocking acquire happens
   in `blit_canvas` after all CPU-side recording. That is usually fine (it overlaps recording
   with the wait), but if profiling shows swapchain stalls, `SDL_AcquireGPUSwapchainTexture`
   (non-blocking, skip frame on failure) or acquiring earlier are options SDL explicitly
   supports. Related: `blit_info.cycle = true` on a swapchain destination is a no-op worth
   removing for clarity.
7. **Pipeline cache growth is unbounded** — keyed on vertex stride/layout too, so apps that
   create many transient meshes with varying layouts accumulate pipelines forever. Not a
   problem for typical CF usage, but an eviction policy (or a cap + warning) would guard it.

---

## 5. Summary

The integration is a clean, conventional SDL_GPU backend: one device, one command buffer per
frame, lazily begun render passes, content-keyed pipeline caching, reflection-driven
name-matched resource binding, and a straightforward compute path. The most impactful fixes
are the **readback submission-order bug (2.1)**, the **storage-buffer resize flag loss (2.2)**,
and the **sampler slot-mapping gap (2.3)** — all small, local patches. The highest-leverage
performance work is reducing per-batch render-pass splitting (4.2) and short-circuiting the
per-draw pipeline lookup (4.1). The largest maintainability win is unifying the triplicated
per-stage reflection state (3.3) and the six hand-rolled copy-pass blocks (3.1).
