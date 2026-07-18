/*
	draw_soa_microbench

	Microbenchmark for the draw/spritebatch path that the SoA opportunity
	report flagged as the only framework-owned layout likely to matter.

	What it measures (per frame, after warmup):
	  - submit_ms : time in cf_draw_* only (push fat BatchGeometry AoS)
	  - batch_ms  : time in cf_render_to only (spritebatch sort + vertex
	                expand + draw; NO window present — this is the SoA target)
	  - present_ms: time in cf_app_draw_onto_screen after an empty command
	                list (blit + swap). Reported so you can see compositor
	                noise separate from batch cost.
	  - cpu_ms    : submit + batch (SoA-relevant total)

	Tiers default to the report's "noticeability" ladder:
	  500 / 2_000 / 10_000  (optional 20_000 with --max-n=20000)

	Kinds:
	  sprites  - fat spritebatch_sprite_t (~320 B) path (primary SoA candidate)
	  quads    - SDF shape path (also fat BatchGeometry)
	  text     - glyph sprites
	  mixed    - alternating sprite / quad / circle / text

	Usage:
	  ./draw_soa_microbench [--csv=path] [--max-n=N] [--min-n=N]
	                        [--kinds=sprites,quads,text,mixed]
	                        [--frames=N] [--warmup=N] [--hidden]

	CSV columns:
	  n,kind,frames,
	  submit_ms_mean,submit_ms_p50,submit_ms_p95,
	  batch_ms_mean,batch_ms_p50,batch_ms_p95,
	  present_ms_mean,present_ms_p50,present_ms_p95,
	  cpu_ms_mean,cpu_ms_p50,cpu_ms_p95
*/

#include <cute.h>

#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum {
	KIND_SPRITES = 0,
	KIND_QUADS,
	KIND_TEXT,
	KIND_MIXED,
	KIND_COUNT
};

static const char* kind_name(int k)
{
	switch (k) {
	case KIND_SPRITES: return "sprites";
	case KIND_QUADS:   return "quads";
	case KIND_TEXT:    return "text";
	case KIND_MIXED:   return "mixed";
	default:           return "?";
	}
}

// ---------------------------------------------------------------------------
// Deterministic positions (precomputed so RNG is not in the timed path).

typedef struct {
	CF_V2* p;
	int capacity;
} PosBuf;

static void pos_buf_fit(PosBuf* b, int n)
{
	if (n <= b->capacity) return;
	free(b->p);
	b->p = (CF_V2*)malloc(sizeof(CF_V2) * (size_t)n);
	b->capacity = n;
}

static void pos_buf_fill(PosBuf* b, int n, float w, float h)
{
	pos_buf_fit(b, n);
	// Halton-ish lattice so draws spread across the atlas / screen.
	for (int i = 0; i < n; ++i) {
		float u = (float)((i * 2654435761u) & 0xFFFF) / 65535.0f;
		float v = (float)((i * 2246822519u) & 0xFFFF) / 65535.0f;
		b->p[i].x = (u - 0.5f) * (w - 16.0f);
		b->p[i].y = (v - 0.5f) * (h - 16.0f);
	}
}

// ---------------------------------------------------------------------------
// Draw submission (timed).

static void submit_draws(int kind, int n, const CF_V2* pos, CF_Sprite sprite)
{
	switch (kind) {
	case KIND_SPRITES:
		for (int i = 0; i < n; ++i) {
			CF_Sprite s = sprite;
			s.transform.p = pos[i];
			cf_draw_sprite(&s);
		}
		break;

	case KIND_QUADS:
		for (int i = 0; i < n; ++i) {
			CF_V2 p = pos[i];
			CF_Aabb bb = cf_make_aabb(cf_v2(p.x - 4.0f, p.y - 4.0f), cf_v2(p.x + 4.0f, p.y + 4.0f));
			cf_draw_quad_fill(bb, 0.0f);
		}
		break;

	case KIND_TEXT:
		for (int i = 0; i < n; ++i) {
			cf_draw_text("X", pos[i], -1);
		}
		break;

	case KIND_MIXED:
		for (int i = 0; i < n; ++i) {
			CF_V2 p = pos[i];
			switch (i & 3) {
			case 0: {
				CF_Sprite s = sprite;
				s.transform.p = p;
				cf_draw_sprite(&s);
			} break;
			case 1: {
				CF_Aabb bb = cf_make_aabb(cf_v2(p.x - 4.0f, p.y - 4.0f), cf_v2(p.x + 4.0f, p.y + 4.0f));
				cf_draw_quad_fill(bb, 0.0f);
			} break;
			case 2:
				cf_draw_circle_fill2(p, 4.0f);
				break;
			case 3:
				cf_draw_text("X", p, -1);
				break;
			}
		}
		break;

	default:
		break;
	}
}

// ---------------------------------------------------------------------------
// Stats helpers.

static int cmp_double(const void* a, const void* b)
{
	double x = *(const double*)a;
	double y = *(const double*)b;
	return (x > y) - (x < y);
}

typedef struct {
	double mean;
	double p50;
	double p95;
} Stats;

static Stats stats_of(double* samples, int count)
{
	Stats s = { 0 };
	if (count <= 0) return s;
	double sum = 0.0;
	for (int i = 0; i < count; ++i) sum += samples[i];
	s.mean = sum / (double)count;
	qsort(samples, (size_t)count, sizeof(double), cmp_double);
	s.p50 = samples[count / 2];
	int i95 = (int)(count * 0.95);
	if (i95 >= count) i95 = count - 1;
	s.p95 = samples[i95];
	return s;
}

static int frames_for_n(int n, int frames_override)
{
	if (frames_override > 0) return frames_override;
	if (n <= 500)   return 120;
	if (n <= 2000)  return 80;
	if (n <= 10000) return 40;
	return 20;
}

// ---------------------------------------------------------------------------

int main(int argc, char* argv[])
{
	const char* csv_path = NULL;
	int min_n = 500;
	int max_n = 10000;
	int frames_override = 0;
	int warmup = 15;
	int kind_mask = (1 << KIND_COUNT) - 1;
	bool hidden = false;

	for (int i = 1; i < argc; ++i) {
		if (strncmp(argv[i], "--csv=", 6) == 0) {
			csv_path = argv[i] + 6;
		} else if (strncmp(argv[i], "--max-n=", 8) == 0) {
			max_n = atoi(argv[i] + 8);
		} else if (strncmp(argv[i], "--min-n=", 8) == 0) {
			min_n = atoi(argv[i] + 8);
		} else if (strncmp(argv[i], "--frames=", 9) == 0) {
			frames_override = atoi(argv[i] + 9);
		} else if (strncmp(argv[i], "--warmup=", 9) == 0) {
			warmup = atoi(argv[i] + 9);
		} else if (strcmp(argv[i], "--hidden") == 0) {
			hidden = true;
		} else if (strncmp(argv[i], "--kinds=", 8) == 0) {
			kind_mask = 0;
			const char* p = argv[i] + 8;
			for (int k = 0; k < KIND_COUNT; ++k) {
				if (strstr(p, kind_name(k))) kind_mask |= (1 << k);
			}
			if (kind_mask == 0) kind_mask = (1 << KIND_COUNT) - 1;
		} else if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
			fprintf(stderr,
				"Usage: %s [--csv=path] [--min-n=N] [--max-n=N]\n"
				"          [--kinds=sprites,quads,text,mixed]\n"
				"          [--frames=N] [--warmup=N] [--hidden]\n",
				argv[0]);
			return 0;
		}
	}

	// Report ladder + optional 20k.
	const int tiers[] = { 500, 2000, 10000, 20000 };
	const int num_tiers = (int)(sizeof(tiers) / sizeof(tiers[0]));

	const float w = 800.0f;
	const float h = 600.0f;
	int options = CF_APP_OPTIONS_WINDOW_POS_CENTERED_BIT | CF_APP_OPTIONS_NO_AUDIO_BIT;
	if (hidden) options |= CF_APP_OPTIONS_HIDDEN_BIT;

	CF_Result result = cf_make_app("draw_soa_microbench", 0, 0, 0, (int)w, (int)h, options, argv[0]);
	if (cf_is_error(result)) {
		fprintf(stderr, "cf_make_app failed: %s\n", result.details ? result.details : "(no details)");
		return 1;
	}
	cf_app_set_vsync(false);

	// One small solid sprite — no disk IO in the timed path.
	CF_Pixel pixels[8 * 8];
	for (int i = 0; i < 8 * 8; ++i) pixels[i] = cf_make_pixel_rgba(255, 255, 255, 255);
	CF_Sprite sprite = cf_make_easy_sprite_from_pixels(pixels, 8, 8);

	FILE* csv = csv_path ? fopen(csv_path, "w") : stdout;
	if (!csv) {
		fprintf(stderr, "could not open csv path: %s\n", csv_path);
		cf_destroy_app();
		return 1;
	}

	fprintf(csv,
		"n,kind,frames,"
		"submit_ms_mean,submit_ms_p50,submit_ms_p95,"
		"batch_ms_mean,batch_ms_p50,batch_ms_p95,"
		"present_ms_mean,present_ms_p50,present_ms_p95,"
		"cpu_ms_mean,cpu_ms_p50,cpu_ms_p95\n");
	fflush(csv);

	fprintf(stderr,
		"[draw_soa_microbench] submit + batch (cf_render_to) + present split\n"
		"[draw_soa_microbench] min_n=%d max_n=%d warmup=%d hidden=%d\n",
		min_n, max_n, warmup, (int)hidden);

	// Dedicated canvas so batch timing is not mixed with the app's default
	// present path. Same resolution as the window.
	CF_CanvasParams canvas_params = cf_canvas_defaults((int)w, (int)h);
	CF_Canvas batch_canvas = cf_make_canvas(canvas_params);

	PosBuf positions = { 0 };
	int exit_code = 0;

	for (int ti = 0; ti < num_tiers; ++ti) {
		int n = tiers[ti];
		if (n < min_n || n > max_n) continue;

		pos_buf_fill(&positions, n, w, h);
		int frames = frames_for_n(n, frames_override);

		for (int k = 0; k < KIND_COUNT; ++k) {
			if (!(kind_mask & (1 << k))) continue;

			fprintf(stderr, "[bench] n=%-6d kind=%-7s frames=%d ...\n", n, kind_name(k), frames);

			double* submit_samples  = (double*)malloc(sizeof(double) * (size_t)frames);
			double* batch_samples   = (double*)malloc(sizeof(double) * (size_t)frames);
			double* present_samples = (double*)malloc(sizeof(double) * (size_t)frames);
			double* cpu_samples     = (double*)malloc(sizeof(double) * (size_t)frames);
			if (!submit_samples || !batch_samples || !present_samples || !cpu_samples) {
				fprintf(stderr, "oom\n");
				exit_code = 1;
				free(submit_samples);
				free(batch_samples);
				free(present_samples);
				free(cpu_samples);
				goto done;
			}

			int sample_i = 0;

			for (int f = 0; f < warmup + frames; ++f) {
				cf_app_update(NULL);
				if (!cf_app_is_running()) {
					fprintf(stderr, "[bench] app closed early\n");
					free(submit_samples);
					free(batch_samples);
					free(present_samples);
					free(cpu_samples);
					goto done;
				}

				// 1) Push draw items (fat AoS construction).
				CF_Stopwatch t_submit = cf_make_stopwatch();
				submit_draws(k, n, positions.p, sprite);
				double submit_ms = cf_stopwatch_milliseconds(t_submit);

				// 2) Sort + expand + draw into offscreen canvas (SoA target path).
				//    Deliberately does NOT present to the window.
				CF_Stopwatch t_batch = cf_make_stopwatch();
				cf_render_to(batch_canvas, true);
				double batch_ms = cf_stopwatch_milliseconds(t_batch);

				// 3) Present an empty frame so the window/compositor still runs.
				//    Commands were already flushed by cf_render_to, so this is
				//    mostly blit + swap cost.
				CF_Stopwatch t_present = cf_make_stopwatch();
				cf_app_draw_onto_screen(false);
				double present_ms = cf_stopwatch_milliseconds(t_present);

				if (f >= warmup) {
					submit_samples[sample_i] = submit_ms;
					batch_samples[sample_i] = batch_ms;
					present_samples[sample_i] = present_ms;
					cpu_samples[sample_i] = submit_ms + batch_ms;
					++sample_i;
				}
			}

			Stats submit  = stats_of(submit_samples, frames);
			Stats batch   = stats_of(batch_samples, frames);
			Stats present = stats_of(present_samples, frames);
			Stats cpu     = stats_of(cpu_samples, frames);

			fprintf(csv,
				"%d,%s,%d,"
				"%.4f,%.4f,%.4f,"
				"%.4f,%.4f,%.4f,"
				"%.4f,%.4f,%.4f,"
				"%.4f,%.4f,%.4f\n",
				n, kind_name(k), frames,
				submit.mean, submit.p50, submit.p95,
				batch.mean, batch.p50, batch.p95,
				present.mean, present.p50, present.p95,
				cpu.mean, cpu.p50, cpu.p95);
			fflush(csv);

			fprintf(stderr,
				"[bench]   submit=%6.3fms (p95 %6.3f) | "
				"batch=%6.3fms (p95 %6.3f) | "
				"present=%6.3fms | "
				"cpu=%6.3fms (p95 %6.3f)\n",
				submit.mean, submit.p95,
				batch.mean, batch.p95,
				present.mean,
				cpu.mean, cpu.p95);

			free(submit_samples);
			free(batch_samples);
			free(present_samples);
			free(cpu_samples);
		}
	}

done:
	free(positions.p);
	cf_destroy_canvas(batch_canvas);
	if (csv_path && csv) fclose(csv);
	cf_destroy_app();
	return exit_code;
}
