#include <cute.h>
using namespace Cute;

// Repro for #624: a zero-width box blanks everything drawn before it in its tiled batch.
// The health bar drains to exactly 0, holds, then refills. Watch the ground at 0 hp.
// T toggles the draw path (auto/tiled vs. forced instanced).

// Internal (src/internal/cute_draw_internal.h).
CF_API void CF_CALL cf_draw_set_tiled_enabled(bool enabled);
CF_API void CF_CALL cf_draw_set_tiled_auto();

int main(int argc, char* argv[])
{
	make_app("Zero-width box (#624)", 0, 0, 0, 640, 480, CF_APP_OPTIONS_WINDOW_POS_CENTERED_BIT, argv[0]);
	float health = 100;
	bool instanced = false;

	while (app_is_running()) {
		app_update();

		if (cf_key_just_pressed(CF_KEY_T)) {
			instanced = !instanced;
			if (instanced) cf_draw_set_tiled_enabled(false);
			else cf_draw_set_tiled_auto();
		}
		health -= 50 * CF_DELTA_TIME;
		if (health < -75) health = 100; // Hold at 0 hp for ~1.5s, then refill.
		float hp = cf_max(health, 0.0f);

		// Ground: big opaque box, so auto mode routes the batch tiled.
		draw_push_color(cf_make_color_rgb_f(0.3f, 0.7f, 0.3f));
		draw_box_fill(make_aabb(V2(-320, -240), V2(320, -40)), 0);
		draw_pop_color();

		// Health bar: exactly zero width at 0 hp.
		draw_push_color(cf_make_color_rgb_f(0.9f, 0.2f, 0.2f));
		draw_box_fill(make_aabb(V2(-50, 60), V2(-50 + hp, 72)), 0);
		draw_pop_color();

		char buf[128];
		snprintf(buf, sizeof(buf), "hp = %.1f   path = %s (T toggles)", hp, instanced ? "instanced" : "auto/tiled");
		draw_text(buf, V2(-300, 200));

		app_draw_onto_screen(true);
	}

	destroy_app();
	return 0;
}
