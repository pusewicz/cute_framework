// Direct test of the render-pass-churn hypothesis from the item-2 investigation.
//
// Finding so far: cf_apply_scissor/cf_apply_shader reuse an already-open SDL_GPU render
// pass (no pass break) -- the ONLY thing that forces a pass break during the churn
// workload is cf_mesh_update_vertex_data's underlying s_update_buffer, which unconditionally
// ends the active render pass because it needs to run a copy-pass, and copy-passes can't
// overlap render-passes on modern GPU APIs. draw_report_bench re-uploads fresh vertex data
// once per batch (via cf_draw_quad_fill -> cute_draw's internal spritebatch flush), forcing
// ~1000 pass-break-and-reopen cycles per frame.
//
// This bench isolates that one variable: N separate tiny meshes (6 verts each) are created
// and uploaded ONCE at setup, outside the timed loop. Each measured frame just applies a
// unique scissor rect and draws each pre-built mesh once -- N draw calls, N scissor changes,
// but ZERO vertex re-uploads and therefore (if the hypothesis holds) zero pass breaks, since
// nothing forces a copy-pass mid-frame. If frame time collapses back near the frame-pacing
// floor, the pass-break/copy-pass interleaving is confirmed as the dominant cost, not raw
// per-draw-call/pipeline-bind overhead. If it's still slow, something else dominates.
//
// Usage: scissor_churn_no_reupload_bench [--n=N] [--frames=N] [--warmup=N] [--rounds=N] [--show]

#include <cute.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct Vertex
{
	CF_V2 pos;
	CF_Pixel color;
} Vertex;

static const char* s_vs =
"layout (location = 0) in vec2 in_pos;\n"
"layout (location = 1) in vec4 in_col;\n"
"layout (location = 0) out vec4 v_col;\n"
"void main()\n"
"{\n"
"    v_col = in_col;\n"
"    gl_Position = vec4(in_pos, 0, 1);\n"
"}\n";

static const char* s_fs =
"layout(location = 0) in vec4 v_col;\n"
"layout(location = 0) out vec4 result;\n"
"void main() { result = v_col; }\n";

static int s_compare_doubles(const void* a, const void* b)
{
	double x = *(const double*)a;
	double y = *(const double*)b;
	return (x > y) - (x < y);
}

static double s_median(double* samples, int count)
{
	qsort(samples, count, sizeof(double), s_compare_doubles);
	return samples[count / 2];
}

static uint32_t s_rng_state = 0x9E3779B9u;
static float s_frand(float lo, float hi)
{
	s_rng_state = s_rng_state * 1664525u + 1013904223u;
	float t = (float)((s_rng_state >> 8) & 0xFFFFFFu) / (float)0xFFFFFFu;
	return lo + t * (hi - lo);
}

int main(int argc, char* argv[])
{
	int n = 1000;
	int frames = 20;
	int warmup = 10;
	int rounds = 3;
	bool show = false;

	for (int i = 1; i < argc; ++i) {
		if (!strncmp(argv[i], "--n=", 4)) n = atoi(argv[i] + 4);
		else if (!strncmp(argv[i], "--frames=", 9)) frames = atoi(argv[i] + 9);
		else if (!strncmp(argv[i], "--warmup=", 9)) warmup = atoi(argv[i] + 9);
		else if (!strncmp(argv[i], "--rounds=", 9)) rounds = atoi(argv[i] + 9);
		else if (!strcmp(argv[i], "--show")) show = true;
		else {
			fprintf(stderr, "Usage: %s [--n=N] [--frames=N] [--warmup=N] [--rounds=N] [--show]\n", argv[0]);
			return -1;
		}
	}

	float w = 1280.0f;
	float h = 720.0f;
	int options = CF_APP_OPTIONS_RESIZABLE_BIT;
	if (!show) options |= CF_APP_OPTIONS_HIDDEN_BIT;
	CF_Result result = cf_make_app("scissor_churn_no_reupload_bench", 0, 0, 0, (int)w, (int)h, options, argv[0]);
	if (cf_is_error(result)) {
		fprintf(stderr, "%s\n", result.details);
		return -1;
	}
	cf_app_set_vsync(false);

	CF_VertexAttribute attrs[2] = { 0 };
	attrs[0].name = "in_pos";
	attrs[0].format = CF_VERTEX_FORMAT_FLOAT2;
	attrs[0].offset = CF_OFFSET_OF(Vertex, pos);
	attrs[1].name = "in_col";
	attrs[1].format = CF_VERTEX_FORMAT_UBYTE4_NORM;
	attrs[1].offset = CF_OFFSET_OF(Vertex, color);

	// Build and upload N tiny meshes ONCE, outside the timed loop. Also precompute each
	// quad's scissor rect (in pixel space, matching cf_apply_scissor's expected units).
	CF_Mesh* meshes = (CF_Mesh*)malloc(sizeof(CF_Mesh) * n);
	CF_Rect* scissors = (CF_Rect*)malloc(sizeof(CF_Rect) * n);
	for (int i = 0; i < n; ++i) {
		CF_V2 p = cf_v2(s_frand(-w * 0.5f, w * 0.5f), s_frand(-h * 0.5f, h * 0.5f));
		float ndc_x = p.x / (w * 0.5f);
		float ndc_y = p.y / (h * 0.5f);
		float ndc_hx = 8.0f / (w * 0.5f);
		float ndc_hy = 8.0f / (h * 0.5f);
		Vertex verts[6] = {
			{ cf_v2(ndc_x - ndc_hx, ndc_y - ndc_hy), cf_pixel_white() },
			{ cf_v2(ndc_x + ndc_hx, ndc_y - ndc_hy), cf_pixel_white() },
			{ cf_v2(ndc_x + ndc_hx, ndc_y + ndc_hy), cf_pixel_white() },
			{ cf_v2(ndc_x - ndc_hx, ndc_y - ndc_hy), cf_pixel_white() },
			{ cf_v2(ndc_x + ndc_hx, ndc_y + ndc_hy), cf_pixel_white() },
			{ cf_v2(ndc_x - ndc_hx, ndc_y + ndc_hy), cf_pixel_white() },
		};
		meshes[i] = cf_make_mesh(sizeof(verts), attrs, 2, sizeof(Vertex));
		cf_mesh_update_vertex_data(meshes[i], verts, 6);

		// Scissor rect around this quad's screen-space position (window is pixel-space
		// with origin at top-left; our world coords are centered at 0,0).
		scissors[i].x = (int)(p.x + w * 0.5f - 16);
		scissors[i].y = (int)(p.y + h * 0.5f - 16);
		scissors[i].w = 32;
		scissors[i].h = 32;
	}

	CF_Material material = cf_make_material();
	CF_Shader shader = cf_make_shader_from_source(s_vs, s_fs);

	fprintf(stderr, "[scissor_churn_no_reupload_bench] n=%d frames=%d warmup=%d rounds=%d hidden=%d\n",
		n, frames, warmup, rounds, (int)!show);

	double* round_medians = (double*)malloc(sizeof(double) * rounds);
	double* samples = (double*)malloc(sizeof(double) * frames);

	for (int r = 0; r < rounds; ++r) {
		for (int f = 0; f < warmup + frames; ++f) {
			cf_app_update(NULL);
			if (!cf_app_is_running()) { fprintf(stderr, "[scissor_churn_no_reupload_bench] app stopped early\n"); return -1; }

			CF_Stopwatch sw = cf_make_stopwatch();

			cf_apply_canvas(cf_app_get_canvas(), true);
			for (int i = 0; i < n; ++i) {
				cf_apply_mesh(meshes[i]);
				cf_apply_shader(shader, material);
				cf_apply_scissor(scissors[i].x, scissors[i].y, scissors[i].w, scissors[i].h);
				cf_draw_elements();
			}
			cf_app_draw_onto_screen(false);

			double ms = cf_stopwatch_milliseconds(sw);
			if (f >= warmup) samples[f - warmup] = ms;
		}

		round_medians[r] = s_median(samples, frames);
		fprintf(stderr, "[scissor_churn_no_reupload_bench] round %d/%d median=%.4fms\n", r + 1, rounds, round_medians[r]);
	}

	double best = round_medians[0];
	for (int r = 1; r < rounds; ++r) {
		if (round_medians[r] < best) best = round_medians[r];
	}

	printf("n=%d frames=%d rounds=%d hidden=%d median_p50_ms=%.4f\n", n, frames, rounds, (int)!show, best);

	free(samples);
	free(round_medians);
	free(meshes);
	free(scissors);
	cf_destroy_shader(shader);
	cf_destroy_material(material);
	cf_destroy_app();
	return 0;
}
