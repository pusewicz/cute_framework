#include <cute.h>
using namespace Cute;

int main(int argc, char* argv[])
{
	CF_Result result = make_app("Basic Shapes", 0, 0, 0, 640, 480, CF_APP_OPTIONS_WINDOW_POS_CENTERED_BIT, argv[0]);
	if (is_error(result)) {
		printf("Error: %s\n", result.details);
		return -1;
	}

	while (app_is_running()) {
		app_update();

		draw_push_color(make_color(0xeba48b));

		float radius = 100.0f;
		float motion = (sinf((float)CF_SECONDS) + 1.0f) * 0.5f * 40.0f;
		draw_circle(V2(0,0), radius + motion, 1.0f + motion / 4);

		draw_push_color(color_purple());
		motion *= 3;
		draw_quad(make_aabb(V2(0,0), 30 + motion, 30 + motion), 5);
		draw_pop_color();

		app_draw_onto_screen(true);
	}

	destroy_app();

	return 0;
}
