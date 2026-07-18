// Headless benchmark for s_draw_report's per-batch fixed overhead (cute_draw.cpp).
//
// Draws N quads per frame, each wrapped in its own push/pop scissor pair. Since a
// scissor change forces a fresh CF_Command (and therefore a spritebatch flush boundary),
// this is a pathological "one broken batch per draw call" workload -- it isolates the
// per-batch fixed cost inside s_draw_report (mesh upload, texture/uniform/render-state/
// sampler/shader/viewport/scissor apply, draw call) from the per-sprite cost that scales
// with well-batched scenes.
//
// Runs with a hidden window by default (pass --show to open one) so it can be compared
// before/after without a visible/foreground window, and reports min-of-rounds median frame
// time to line up with this project's existing benchmarking convention (frame times on
// Apple Silicon are bimodal -- P-core/E-core/compositor throttling -- so a single round's
// mean or even median can mislead; take the best of several rounds instead).
//
// Usage: draw_report_bench [--n=N] [--frames=N] [--warmup=N] [--rounds=N] [--show]

#include <cute.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

static void s_draw_one_broken_batch(float world_w, float world_h)
{
	CF_V2 p = cf_v2(s_frand(-world_w * 0.5f, world_w * 0.5f), s_frand(-world_h * 0.5f, world_h * 0.5f));
	CF_Rect scissor = { (int)(p.x + world_w * 0.5f - 16), (int)(p.y + world_h * 0.5f - 16), 32, 32 };
	cf_draw_push_scissor(scissor);
	CF_Aabb bb = cf_make_aabb(cf_v2(p.x - 4, p.y - 4), cf_v2(p.x + 4, p.y + 4));
	cf_draw_quad_fill(bb, 1.0f);
	cf_draw_pop_scissor();
}

int main(int argc, char* argv[])
{
	int n = 1000;
	int frames = 60;
	int warmup = 15;
	int rounds = 5;
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

	float w = 640.0f;
	float h = 480.0f;
	int options = CF_APP_OPTIONS_RESIZABLE_BIT;
	if (!show) options |= CF_APP_OPTIONS_HIDDEN_BIT;
	CF_Result result = cf_make_app("draw_report_bench", 0, 0, 0, (int)w, (int)h, options, argv[0]);
	if (cf_is_error(result)) {
		fprintf(stderr, "%s\n", result.details);
		return -1;
	}
	cf_app_set_vsync(false);

	fprintf(stderr, "[draw_report_bench] n=%d frames=%d warmup=%d rounds=%d hidden=%d\n",
		n, frames, warmup, rounds, (int)!show);

	double* round_medians = (double*)malloc(sizeof(double) * rounds);
	double* samples = (double*)malloc(sizeof(double) * frames);
	double* issue_samples = (double*)malloc(sizeof(double) * frames);
	double* flush_samples = (double*)malloc(sizeof(double) * frames);

	for (int r = 0; r < rounds; ++r) {
		for (int f = 0; f < warmup + frames; ++f) {
			cf_app_update(NULL);
			if (!cf_app_is_running()) {
				fprintf(stderr, "[draw_report_bench] app stopped running early\n");
				return -1;
			}

			CF_Stopwatch sw = cf_make_stopwatch();

			CF_Stopwatch issue_sw = cf_make_stopwatch();
			for (int i = 0; i < n; ++i) {
				s_draw_one_broken_batch(w, h);
			}
			double issue_ms = cf_stopwatch_milliseconds(issue_sw);

			CF_Stopwatch flush_sw = cf_make_stopwatch();
			cf_app_draw_onto_screen(true);
			double flush_ms = cf_stopwatch_milliseconds(flush_sw);

			double ms = cf_stopwatch_milliseconds(sw);
			if (f >= warmup) {
				samples[f - warmup] = ms;
				issue_samples[f - warmup] = issue_ms;
				flush_samples[f - warmup] = flush_ms;
			}
		}

		round_medians[r] = s_median(samples, frames);
		double issue_median = s_median(issue_samples, frames);
		double flush_median = s_median(flush_samples, frames);
		fprintf(stderr, "[draw_report_bench] round %d/%d median=%.4fms (issue=%.4fms flush=%.4fms)\n",
			r + 1, rounds, round_medians[r], issue_median, flush_median);
	}

	double best = round_medians[0];
	for (int r = 1; r < rounds; ++r) {
		if (round_medians[r] < best) best = round_medians[r];
	}

	printf("n=%d frames=%d rounds=%d hidden=%d median_p50_ms=%.4f\n", n, frames, rounds, (int)!show, best);

	free(samples);
	free(round_medians);
	cf_destroy_app();
	return 0;
}
