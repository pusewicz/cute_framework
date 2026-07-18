// draw_bench: Phase 0 baseline benchmark for the cute_draw batcher.
//
// Sweeps a fixed matrix of (draw_count, draw_kind) and records per-frame
// stats (wall-clock ms, GPU draw calls) to a CSV. Runs in a visible window
// with vsync disabled so we measure raw CPU+GPU cost, not display cadence.
//
// CSV is emitted to stdout, suitable for redirection. The same binary is
// re-run from inside the worktree before AND after each refactor phase.

#include <cute.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>

typedef enum {
	KIND_SPRITES,
	KIND_QUADS,
	KIND_CIRCLES,
	KIND_TEXT,
	KIND_MIXED,
	KIND_CHURN,
	KIND_COUNT
} Kind;

static const char* kind_name(Kind k)
{
	switch (k) {
	case KIND_SPRITES: return "sprites";
	case KIND_QUADS:   return "quads";
	case KIND_CIRCLES: return "circles";
	case KIND_TEXT:    return "text";
	case KIND_MIXED:   return "mixed";
	case KIND_CHURN:   return "churn";
	default:           return "?";
	}
}

static uint32_t g_lcg = 0x12345678u;
static float frand(float lo, float hi)
{
	g_lcg = g_lcg * 1103515245u + 12345u;
	float t = (float)((g_lcg >> 8) & 0xFFFFFFu) / (float)0xFFFFFFu;
	return lo + t * (hi - lo);
}

static int compare_double(const void* a, const void* b)
{
	double x = *(const double*)a;
	double y = *(const double*)b;
	return (x > y) - (x < y);
}

static void run_kind(Kind kind, int N, float w, float h, CF_Sprite sprite)
{
	float xlo = -w * 0.5f + 4.0f;
	float xhi =  w * 0.5f - 4.0f;
	float ylo = -h * 0.5f + 4.0f;
	float yhi =  h * 0.5f - 4.0f;

	switch (kind) {
	case KIND_SPRITES:
		for (int i = 0; i < N; ++i) {
			CF_Sprite s = sprite;
			s.transform.p = cf_v2(frand(xlo, xhi), frand(ylo, yhi));
			cf_draw_sprite(&s);
		}
		break;
	case KIND_QUADS:
		for (int i = 0; i < N; ++i) {
			CF_V2 p = cf_v2(frand(xlo, xhi), frand(ylo, yhi));
			CF_Aabb bb = cf_make_aabb(cf_v2(p.x - 4, p.y - 4), cf_v2(p.x + 4, p.y + 4));
			cf_draw_quad_fill(bb, 1.0f);
		}
		break;
	case KIND_CIRCLES:
		for (int i = 0; i < N; ++i) {
			CF_V2 p = cf_v2(frand(xlo, xhi), frand(ylo, yhi));
			cf_draw_circle_fill2(p, 4.0f);
		}
		break;
	case KIND_TEXT:
		for (int i = 0; i < N; ++i) {
			CF_V2 p = cf_v2(frand(xlo, xhi), frand(ylo, yhi));
			cf_draw_text("X", p, -1);
		}
		break;
	case KIND_MIXED:
		for (int i = 0; i < N; ++i) {
			CF_V2 p = cf_v2(frand(xlo, xhi), frand(ylo, yhi));
			switch (i % 5) {
			case 0: {
				CF_Sprite s = sprite;
				s.transform.p = p;
				cf_draw_sprite(&s);
			} break;
			case 1: {
				CF_Aabb bb = cf_make_aabb(cf_v2(p.x - 4, p.y - 4), cf_v2(p.x + 4, p.y + 4));
				cf_draw_quad_fill(bb, 1.0f);
			} break;
			case 2:
				cf_draw_circle_fill2(p, 4.0f);
				break;
			case 3:
				cf_draw_text("X", p, -1);
				break;
			case 4:
				cf_draw_line(p, cf_v2(p.x + 8, p.y + 8), 1.0f);
				break;
			}
		}
		break;
	case KIND_CHURN:
		// Each draw is wrapped in its own push/pop scissor pair, forcing
		// a fresh CF_Command (and therefore a flush boundary) per draw.
		// Intentionally pathological — exposes the cmd-stream overhead.
		for (int i = 0; i < N; ++i) {
			CF_V2 p = cf_v2(frand(xlo, xhi), frand(ylo, yhi));
			CF_Rect r = { (int)(p.x + w*0.5f - 16), (int)(p.y + h*0.5f - 16), 32, 32 };
			cf_draw_push_scissor(r);
			CF_Aabb bb = cf_make_aabb(cf_v2(p.x - 4, p.y - 4), cf_v2(p.x + 4, p.y + 4));
			cf_draw_quad_fill(bb, 1.0f);
			cf_draw_pop_scissor();
		}
		break;
	default: break;
	}
}

static int frames_for_tier(int N)
{
	// Hold roughly constant total work across tiers. The 1M tier still
	// gets at least a handful of samples so p95 is meaningful.
	if (N <= 1000)    return 120;
	if (N <= 10000)   return 60;
	if (N <= 100000)  return 20;
	return 8; // 1M
}

int main(int argc, char* argv[])
{
	const char* csv_path = NULL;
	int max_tier = 10000; // Hard cap by default — see --allow-crash-tier.
	int min_tier = 100;
	int kind_mask = -1; // all kinds
	bool allow_crash_tier = false;

	for (int i = 1; i < argc; ++i) {
		if (strncmp(argv[i], "--csv=", 6) == 0) csv_path = argv[i] + 6;
		else if (strncmp(argv[i], "--max-tier=", 11) == 0) max_tier = atoi(argv[i] + 11);
		else if (strncmp(argv[i], "--min-tier=", 11) == 0) min_tier = atoi(argv[i] + 11);
		else if (strcmp(argv[i], "--allow-crash-tier") == 0) allow_crash_tier = true;
		else if (strncmp(argv[i], "--kinds=", 8) == 0) {
			// comma-separated list of kind names
			kind_mask = 0;
			const char* p = argv[i] + 8;
			for (int k = 0; k < KIND_COUNT; ++k) {
				if (strstr(p, kind_name((Kind)k))) kind_mask |= (1 << k);
			}
		}
	}

	// Pre-refactor batcher allocates ~912 bytes per quad in the vertex buffer
	// and a full memset over that range every flush; at >= 100k draws this
	// has been observed to lock up macOS hard. Require an opt-in to run
	// the dangerous tiers.
	if (max_tier > 10000 && !allow_crash_tier) {
		fprintf(stderr, "[bench] refusing to run tiers > 10k without --allow-crash-tier "
			"(pre-refactor 100k+ has crashed the host).\n");
		max_tier = 10000;
	}

	int tiers[] = { 100, 1000, 10000, 100000, 1000000 };
	int num_tiers = (int)(sizeof(tiers) / sizeof(tiers[0]));

	float w = 800.0f;
	float h = 600.0f;
	int options = CF_APP_OPTIONS_WINDOW_POS_CENTERED_BIT | CF_APP_OPTIONS_NO_AUDIO_BIT;
	CF_Result result = cf_make_app("draw_bench", 0, 0, 0, (int)w, (int)h, options, argv[0]);
	if (cf_is_error(result)) {
		fprintf(stderr, "cf_make_app failed: %s\n", result.details);
		return 1;
	}
	cf_app_set_vsync(false);

	// Pre-build a 16x16 white pixel sprite so sprite draws don't need an asset.
	CF_Pixel pixels[16 * 16];
	for (int i = 0; i < 16 * 16; ++i) pixels[i] = cf_make_pixel_rgba(255, 255, 255, 255);
	CF_Sprite sprite = cf_make_easy_sprite_from_pixels(pixels, 16, 16);

	FILE* csv = csv_path ? fopen(csv_path, "w") : stdout;
	if (!csv) {
		fprintf(stderr, "could not open %s for write\n", csv_path);
		return 1;
	}

	fprintf(csv, "tier,kind,frames_sampled,frame_ms_mean,frame_ms_min,frame_ms_p50,frame_ms_p95,frame_ms_max,draw_calls_per_frame\n");
	fflush(csv);

	for (int ti = 0; ti < num_tiers; ++ti) {
		int N = tiers[ti];
		if (N < min_tier || N > max_tier) continue;
		int frames = frames_for_tier(N);

		for (int k = 0; k < KIND_COUNT; ++k) {
			if (!(kind_mask & (1 << k))) continue;
			Kind kind = (Kind)k;

			// Churn pushes one cmd per draw — at N=1M that's 1M cmds per
			// frame, which is the entire batcher overhead test, not a
			// realistic workload. Cap it.
			int run_N = (kind == KIND_CHURN && N > 2000) ? 2000 : N;

			fprintf(stderr, "[bench] starting N=%-7d kind=%-7s frames=%d...\n",
				run_N, kind_name(kind), frames);

			const int warmup_frames = 10;
			double* samples = (double*)malloc(sizeof(double) * frames);
			int dc_total = 0;

			for (int f = 0; f < warmup_frames + frames; ++f) {
				cf_app_update(NULL);
				if (!cf_app_is_running()) goto done;

				CF_Stopwatch sw = cf_make_stopwatch();
				run_kind(kind, run_N, w, h, sprite);
				int dc = cf_app_draw_onto_screen(true);
				double frame_ms = cf_stopwatch_milliseconds(sw);

				if (f >= warmup_frames) {
					samples[f - warmup_frames] = frame_ms;
					dc_total += dc;
				}
			}

			qsort(samples, frames, sizeof(double), compare_double);
			double sum = 0;
			for (int i = 0; i < frames; ++i) sum += samples[i];
			double mean = sum / frames;
			double p50 = samples[frames / 2];
			double p95 = samples[(int)(frames * 0.95)];
			double mn  = samples[0];
			double mx  = samples[frames - 1];
			double dc_per_frame = (double)dc_total / (double)frames;

			fprintf(csv, "%d,%s,%d,%.4f,%.4f,%.4f,%.4f,%.4f,%.2f\n",
				run_N, kind_name(kind), frames, mean, mn, p50, p95, mx, dc_per_frame);
			fflush(csv);

			fprintf(stderr, "[bench]   done N=%-7d kind=%-7s mean=%7.3fms p95=%7.3fms dc=%6.1f\n",
				run_N, kind_name(kind), mean, p95, dc_per_frame);

			free(samples);
		}
	}

done:
	if (csv_path && csv) fclose(csv);
	cf_destroy_app();
	return 0;
}
