#include <cute.h>
#include <math.h>
#ifdef CF_EMSCRIPTEN
#include <emscripten.h>
#endif

using namespace Cute;

static CF_Sprite girl;

static void update()
{
	app_update();

	const CF_DrawFilterMode modes[4] = {
		CF_DRAW_FILTER_NEAREST,
		CF_DRAW_FILTER_LINEAR,
		CF_DRAW_FILTER_SMOOTH,
		CF_DRAW_FILTER_PIXELART,
	};
	const char* labels[4] = { "NEAREST", "LINEAR", "SMOOTH", "PIXELART" };

	// Slowly grow then shrink, so every scale between min and max is held briefly
	// and the difference between filters is easy to read by eye.
	const float min_scale = 0.5f;
	const float max_scale = 8.0f;
	const float seconds_per_half_cycle = 10.0f;
	float t = fmodf((float)CF_SECONDS / seconds_per_half_cycle, 2.0f);
	float tri = t < 1.0f ? t : 2.0f - t;
	float scale = min_scale + tri * (max_scale - min_scale);

	const float pane_x[4] = { -240.0f, -80.0f, 80.0f, 240.0f };
	const float label_y = 200.0f;
	const float scale_text_y = -200.0f;

	for (int i = 0; i < 4; ++i) {
		draw_text(labels[i], V2(pane_x[i] - text_width(labels[i]) * 0.5f, label_y));

		draw_push_filter(modes[i]);
		girl.transform.p = V2(pane_x[i], 0);
		girl.scale = V2(scale, scale);
		sprite_update(girl);
		sprite_draw(girl);
		draw_pop_filter();
	}

	char buf[64];
	snprintf(buf, sizeof(buf), "scale = %.3fx", (double)scale);
	draw_text(buf, V2(-text_width(buf) * 0.5f, scale_text_y));

	app_draw_onto_screen(true);
}

int main(int argc, char* argv[])
{
	CF_Result result = make_app("Filter Modes", 0, 0, 0, 640, 480,
		CF_APP_OPTIONS_WINDOW_POS_CENTERED_BIT | CF_APP_OPTIONS_GFX_DEBUG_BIT,
		argv[0]);
	if (is_error(result)) return -1;

	girl = cf_make_demo_sprite();
	sprite_play(girl, "spin");

#ifdef CF_EMSCRIPTEN
	emscripten_set_main_loop(update, 60, true);
#else
	while (app_is_running()) {
		update();
	}
	destroy_app();
#endif

	return 0;
}
