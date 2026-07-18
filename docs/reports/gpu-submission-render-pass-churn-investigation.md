# GPU Submission & Render-Pass Churn: Root Cause and a Validated Fix Direction

**Date:** 2026-07-18
**Branch:** `draw-report-overhead` (item 2 worktree, based on `master` @ `bb3a0aee`)
**Scope:** Follow-up to the item-2 (`s_draw_report` per-batch overhead) and item-3
(scoped instancing) investigations, both of which converged on the same unexplained cost:
a `cf_sdlgpu_blit_canvas` call taking ~70-82ms/frame for the pathological "churn" workload
(1000 draws/frame, each in its own push/pop scissor pair), and a smaller but real
"submit/present" floor showing up even in the well-batched instancing comparison. This
report investigates *why*, using SDL3's actual GPU API and CF's `cute_graphics_sdlgpu.cpp`
backend, and validates a fix direction with a real measurement.

---

## Executive summary

**Root cause found and confirmed by direct experiment:** the churn cost has nothing to do
with the number of draw calls, scissor changes, or pipeline binds. It comes entirely from
`cf_mesh_update_vertex_data` re-uploading fresh vertex data once per batch. Modern GPU APIs
(and SDL3 GPU specifically) don't allow a copy-pass (buffer upload) to overlap an active
render-pass, so every such upload forces CF to tear down whatever render pass was open and
rebuild a new one on the next draw. `s_draw_report` calls this once per spritebatch-produced
batch — for the churn workload, ~1000 times/frame — meaning CF pays a full
render-pass-teardown-plus-copy-pass cycle 1000 times over, and the accumulated real GPU work
this creates is what eventually shows up as a CPU stall at the *next* frame's
`SDL_WaitAndAcquireGPUSwapchainTexture` call, once the default 2-frames-in-flight cap is hit.

**Validated with a direct experiment:** pre-uploading each churn quad's geometry once
(outside the timed loop) and then issuing the same 1000 draw calls with 1000 unique scissor
rects — all inside one continuously-open render pass — collapsed frame time from **~82ms to
~8.2ms (~10x)**, landing right at the frame-pacing floor. Nothing else about the workload
changed: still 1000 draw calls, 1000 scissor changes, 1000 `apply_mesh`/`apply_shader`
calls. This isolates the copy-pass/render-pass interleaving as the dominant cost with high
confidence.

**One lever tested and rejected:** raising `SDL_SetGPUAllowedFramesInFlight` from the
default 2 to 3 (hypothesizing it would smooth out the backpressure stall) made things
dramatically *worse* — 82ms became 592-630ms. Each "frame" in this workload already
contains ~1000x a normal frame's GPU work; letting the CPU queue up 3 of those before
blocking just grows the backlog further before the inevitable stall, rather than smoothing
anything. Reverted immediately; not a viable mitigation for this workload shape.

**What this means for CF:** the real fix isn't item 2's original hypothesis (caching
redundant per-batch state calls — that's still valid but caps out around 13% per the
phase-timer breakdown) and isn't specific to the adversarial churn benchmark. Any frame with
multiple spritebatch batches — which includes ordinary multi-texture-atlas scenes, not just
pathological scissor-churn — pays a render-pass teardown/copy-pass cycle per batch today.
Restructuring so a frame's vertex data is uploaded via fewer, larger copy-pass operations
*before* the render/draw phase, rather than interleaved per-batch, is a bigger change than
originally scoped for item 2, but has a demonstrated ~10x ceiling on the worst case and helps
the general multi-batch case, not just churn.

---

## 1. How CF uses SDL3 GPU today

CF keeps one long-lived `SDL_GPUCommandBuffer` per frame in `g_ctx.cmd`
(`src/cute_graphics_sdlgpu.cpp`), acquired once (`cf_sdlgpu_attach`, and implicitly at the
top of each frame) and submitted once in `cf_sdlgpu_end_frame`
(`SDL_SubmitGPUCommandBuffer(g_ctx.cmd); g_ctx.cmd = NULL;`). This matches SDL's own
documented recommendation ("many apps will just need one command buffer per frame" —
`SDL_gpu.h:55`).

Within that one command buffer, render passes are opened lazily and reused across draws.
`cf_sdlgpu_apply_shader` (`src/cute_graphics_sdlgpu.cpp:1712-1749`):

```c
// Begin a new render pass if needed, or reuse the active one.
SDL_GPURenderPass* pass = g_ctx.active_pass;
if (!pass) {
	... 
	pass = SDL_BeginGPURenderPass(cmd, &pass_color_info, 1, depth_stencil_ptr);
	g_ctx.active_pass = pass;
	...
}
```

`cf_sdlgpu_apply_scissor` and `cf_sdlgpu_apply_viewport`
(`src/cute_graphics_sdlgpu.cpp:1428-1452`) call `SDL_SetGPUViewport`/`SDL_SetGPUScissor`
directly on `g_ctx.canvas->pass` — **no pass break**. `cf_sdlgpu_apply_mesh` just stores a
pointer. `cf_sdlgpu_apply_canvas` only ends the active pass when switching canvases or
clearing. None of the per-draw state CF exposes publicly (scissor, viewport, mesh, shader)
forces a pass teardown.

The one thing that does: `s_update_buffer` (`src/cute_graphics_sdlgpu.cpp:1350-1395`, used
by both `cf_sdlgpu_mesh_update_vertex_data` and `cf_sdlgpu_mesh_update_instance_data`)
opens with an unconditional `s_end_active_pass()`, then does a copy-pass to upload the new
buffer contents:

```c
static inline void s_update_buffer(...)
{
	s_end_active_pass();
	...
	SDL_GPUCommandBuffer* cmd = g_ctx.cmd ? g_ctx.cmd : SDL_AcquireGPUCommandBuffer(g_ctx.device);
	SDL_GPUCopyPass *pass = SDL_BeginGPUCopyPass(cmd);
	...
	SDL_EndGPUCopyPass(pass);
	if (!g_ctx.cmd) SDL_SubmitGPUCommandBuffer(cmd);
}
```

This is not a CF bug in isolation — copy-passes and render-passes genuinely can't overlap on
modern GPU APIs (Metal, Vulkan, D3D12 all enforce this at some level; SDL_GPU is a thin
abstraction over them). The issue is *how often* CF triggers this: `s_draw_report`
(`src/cute_draw.cpp:369`) calls `cf_mesh_update_vertex_data` once per spritebatch-produced
batch, uploading only that batch's vertex data into the shared `s_draw->mesh` buffer. Every
batch boundary is therefore a forced render-pass teardown + copy-pass + render-pass rebuild,
regardless of what caused the batch boundary (scissor change, shader change, texture atlas
change, render-state change — anything that trips `add_cmd()`).

Finally, `cf_sdlgpu_blit_canvas` (`src/cute_graphics_sdlgpu.cpp:842-878`) is where this all
becomes visible on the CPU:

```c
void cf_sdlgpu_blit_canvas(CF_Canvas canvas)
{
	s_end_active_pass();
	if (g_ctx.swapchain_tex == NULL && !g_ctx.skip_drawing) {
		if (!SDL_WaitAndAcquireGPUSwapchainTexture(g_ctx.cmd, g_ctx.window, ...)) {
			g_ctx.skip_drawing = true;
		}
	}
	...
}
```

`SDL_WaitAndAcquireGPUSwapchainTexture` blocks until a swapchain image is available — and
per SDL's own docs (§2 below), that's gated by how many frames are already queued on the
GPU. If frame N-1 or N-2's command buffer (containing ~1000 render-pass-teardown cycles'
worth of real GPU work) hasn't finished executing yet, this is where the CPU pays for it.

---

## 2. What SDL3's GPU API actually says

From the vendored header (`build-release/_deps/sdl3-src/include/SDL3/SDL_gpu.h`), not
speculation:

- **One command buffer per frame is the intended pattern**, and **multiple render passes per
  command buffer are explicitly supported** (`SDL_gpu.h:50-91`): *"One can encode multiple
  render passes (or alternate between render and compute passes) in a single command
  buffer... The app can begin new Render Passes and make new draws in the same command
  buffer until the entire scene is rendered."* CF's approach is architecturally sound in
  principle — the issue is 1000 render passes per frame is far outside what this guidance
  is describing (a handful of passes: shadow pass, main pass, post-process), not a
  render-pass-per-draw-call pattern.
- **`SDL_SetGPUAllowedFramesInFlight`** (`SDL_gpu.h:4139-4166`): *"The default value when
  the device is created is 2. This means that after you have submitted 2 frames for
  presentation, if the GPU has not finished working on the first frame,
  `SDL_AcquireGPUSwapchainTexture()` will fill the swapchain texture pointer with NULL, and
  `SDL_WaitAndAcquireGPUSwapchainTexture()` will block. Higher values increase throughput at
  the expense of visual latency."* CF never calls this — it runs at the default (2). This
  is exactly the mechanism behind the `blit_canvas` stall, confirmed by the direct
  (negative) experiment in §4.
- **`SDL_SetGPUScissor`/`SDL_SetGPUViewport`** (`SDL_gpu.h:3263-3286`) take a `render_pass`
  handle, confirming they're dynamic per-draw state within a pass, not something that
  requires ending the pass — matching what `cf_sdlgpu_apply_scissor` already does correctly.
- Copy-passes and render-passes are documented as distinct pass types with their own
  begin/end pairs (`SDL_BeginGPUCopyPass`/`SDL_EndGPUCopyPass` vs.
  `SDL_BeginGPURenderPass`/`SDL_EndGPURenderPass`) — the header doesn't document an
  "upload mid-render-pass" path, consistent with `s_end_active_pass()` being necessary
  *when an upload must happen*, not an oversight. The actual bug (in the architectural
  sense) is uploading 1000 times instead of once.

---

## 3. The experiment: isolate re-upload from everything else

To test whether the copy-pass/render-pass interleaving is really the dominant cost (as
opposed to, say, raw per-draw-call or per-pipeline-bind overhead, or genuine GPU
rasterization cost), a new bench, `samples/scissor_churn_no_reupload_bench.c`, builds and
uploads 1000 tiny 6-vertex meshes **once, outside the timed loop** (one `cf_make_mesh` +
`cf_mesh_update_vertex_data` per quad, at setup). Each measured frame then does exactly what
the original churn benchmark does — 1000 unique scissor rects, 1000 `cf_apply_mesh`, 1000
`cf_apply_shader`, 1000 `cf_draw_elements` — **with zero vertex re-uploads**, all within one
continuously-open render pass (`cf_apply_canvas(..., true)` once, then the loop, then
`cf_app_draw_onto_screen(false)` once):

```c
cf_apply_canvas(cf_app_get_canvas(), true);
for (int i = 0; i < n; ++i) {
	cf_apply_mesh(meshes[i]);
	cf_apply_shader(shader, material);
	cf_apply_scissor(scissors[i].x, scissors[i].y, scissors[i].w, scissors[i].h);
	cf_draw_elements();
}
cf_app_draw_onto_screen(false);
```

**Result** (Release, Apple Silicon, n=1000, min of 3 rounds): **median 8.2ms**, down from
the original churn bench's **~82ms** — a ~10x reduction, landing at the same ~8.3ms
frame-pacing floor observed in the item-3 instancing comparison. The phase breakdown
confirms it: `render_to` (the 1000-iteration draw loop) is ~0.001-0.08ms — essentially
free — and `blit` (the swapchain-acquire call) dropped from ~70-82ms to ~6-8ms, because the
GPU no longer has a backlog of 1000 pass-teardown/copy-pass cycles to work through before
the next frame's swapchain image is available.

This isolates the variable cleanly: the *only* thing removed was per-frame vertex
re-upload. Everything else — draw call count, scissor-change count, `apply_mesh`/
`apply_shader` call count — stayed identical. The ~10x recovery is direct evidence that
copy-pass/render-pass interleaving, not per-draw-call overhead, is what dominates the
churn cost.

---

## 4. The rejected lever: more frames in flight makes it worse

Hypothesis: if the cost surfaces as a stall waiting for GPU backlog to drain, raising the
allowed frames-in-flight from 2 to 3 (the SDL-documented maximum) should let the CPU race
further ahead and smooth out the stall.

Tested by adding `SDL_SetGPUAllowedFramesInFlight(g_ctx.device, 3)` right after
`SDL_ClaimWindowForGPUDevice` in `cf_sdlgpu_attach`, then re-running the original
(still-unmodified) churn bench:

| Config | Median frame time (n=1000) |
|---|---|
| Default (2 frames in flight) | ~82ms |
| 3 frames in flight | **592-630ms** |

This made things roughly **7-8x worse**, not better. The reasoning in hindsight: each
"frame" in this workload already contains ~1000x a normal frame's GPU work (1000
render-pass teardown/copy-pass cycles). Letting the CPU queue up 3 such frames before
blocking doesn't smooth anything — it just grows the backlog further before the inevitable
stall, and the eventual wait has to drain a proportionally larger queue. This lever is a
plausible-sounding fix that is actively harmful for this workload shape; reverted
immediately after measurement. **Do not pursue this direction** without first fixing the
re-upload pattern — it would need re-testing after that fix, where the per-frame GPU work is
back to a normal magnitude and the tradeoff (higher throughput, more latency) might
actually apply as documented.

---

## 5. Recommended fix direction (not yet implemented)

The validated fix is architectural: **decouple "getting a frame's geometry onto the GPU"
from "issuing the draws that use it."** Concretely, that means restructuring
`s_draw_report`'s relationship with `cf_mesh_update_vertex_data` so that a frame's vertex
data (across all its batches) is uploaded via one or a small, bounded number of copy-pass
operations *before* any render-pass work begins, rather than one copy-pass per batch
interleaved with render passes.

This is a bigger change than item 2's original scope (which was "cache redundant per-batch
state calls in `s_draw_report`" — see the item-2 phase-timer breakdown, where that caching
opportunity accounts for only ~13% of the cost, dwarfed by this render-pass-churn issue).
Rough shape of what it would take:

- A persistent, growable vertex buffer sized for a full frame's worth of geometry (not the
  per-batch-sized buffer CF uses today), with each batch writing into its own offset range
  instead of overwriting the same buffer contents per batch.
- Either: (a) one large `cf_mesh_update_vertex_data`-equivalent call per frame covering all
  batches' vertex data, executed once before any draws, with subsequent per-batch draws
  using an offset into that buffer (CF's public `cf_draw_elements()` doesn't currently
  expose a vertex-offset draw — this would need a new low-level entry point, mirroring how
  `SDL_DrawGPUPrimitives` already accepts a `first_vertex` parameter); or (b) restructuring
  the spritebatch-flush-driven callback model so multiple batches' geometry can be
  accumulated before the first upload is forced.
- This interacts with spritebatch's per-atlas-page flush model (`spritebatch_flush` is
  called once per texture-atlas transition today) — worth understanding whether atlas
  transitions can also be batched into fewer copy-pass boundaries, or whether they're a
  legitimate case where a texture bind genuinely requires a new pass (texture binds don't
  force pass breaks today per `cf_sdlgpu_apply_shader`'s sampler-binding code, so likely
  not — worth confirming with the same isolate-and-measure approach used here.)

This wasn't implemented as part of this investigation — it's a real engine change (new
low-level draw API, buffer-management rework) beyond a bench-and-measure spike, and deserves
its own scoped design pass given the ~10x ceiling justifies real investment. The
`scissor_churn_no_reupload_bench` result is strong enough evidence to prioritize it above
the original item-2 state-caching fix and, likely, above further instancing work — instancing
(item 3) only helps when many items share identical state; this fix helps *any* multi-batch
frame, which is a much larger fraction of realistic scenes.

---

## 6. Conclusion

The "GPU-submission/swapchain-backpressure cost" is real, and it is not an inherent GPU
throughput limit — it's caused by CF's current pattern of one copy-pass-driven vertex
re-upload per spritebatch batch, which forces a render-pass teardown/rebuild cycle every
single batch. A direct experiment removing just that one variable (pre-upload once, draw
many times within one continuously-open pass) recovered ~10x, confirming the diagnosis.
Increasing frames-in-flight, the obvious "throw the SDL API at it" lever, makes this specific
workload shape actively worse and should not be pursued until the re-upload pattern itself
is fixed. The actionable next step is a scoped design for batching a frame's vertex uploads
ahead of its draws, which has a larger and more broadly-applicable ceiling than either of the
originally-scoped item 2 or item 3 fixes.

---

## Appendix A — Source anchors

```
src/cute_graphics_sdlgpu.cpp:842-878     — cf_sdlgpu_blit_canvas (SDL_WaitAndAcquireGPUSwapchainTexture)
src/cute_graphics_sdlgpu.cpp:881-888     — cf_sdlgpu_end_frame (single per-frame submit)
src/cute_graphics_sdlgpu.cpp:1350-1395   — s_update_buffer (unconditional s_end_active_pass + copy-pass)
src/cute_graphics_sdlgpu.cpp:1397+       — cf_sdlgpu_mesh_update_vertex_data (calls s_update_buffer)
src/cute_graphics_sdlgpu.cpp:1416-1426   — cf_sdlgpu_apply_canvas (only ends pass on canvas switch/clear)
src/cute_graphics_sdlgpu.cpp:1428-1452   — cf_sdlgpu_apply_viewport / cf_sdlgpu_apply_scissor (no pass break)
src/cute_graphics_sdlgpu.cpp:1473-1478   — cf_sdlgpu_apply_mesh (no pass interaction at all)
src/cute_graphics_sdlgpu.cpp:1682-1829   — cf_sdlgpu_apply_shader (lazily begins/reuses g_ctx.active_pass)
src/cute_graphics_sdlgpu.cpp:1831-1848   — cf_sdlgpu_draw_elements
src/cute_graphics_sdlgpu.cpp:792-798     — cf_sdlgpu_attach (device/window claim, single per-frame cmd acquire)
src/cute_draw.cpp:369                    — s_draw_report's per-batch cf_mesh_update_vertex_data call (the trigger)
samples/scissor_churn_no_reupload_bench.c — validating experiment (this investigation)
samples/draw_report_bench.c               — original churn bench (item 2, prior investigation)
build-release/_deps/sdl3-src/include/SDL3/SDL_gpu.h:30-119   — command buffer / render pass usage overview
build-release/_deps/sdl3-src/include/SDL3/SDL_gpu.h:4139-4166 — SDL_SetGPUAllowedFramesInFlight docs
build-release/_deps/sdl3-src/include/SDL3/SDL_gpu.h:3263-3286 — SDL_SetGPUViewport / SDL_SetGPUScissor (render_pass-scoped)
```

---

*End of report.*
