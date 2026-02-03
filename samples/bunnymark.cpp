#include <cute.h>
#include <ctime>
using namespace Cute;

// Constants
constexpr int DISPLAY_WIDTH = 720;
constexpr int DISPLAY_HEIGHT = 720;
constexpr int SPAWN_COUNT = 1000;
constexpr int NUM_BUNNY_TYPES = 12;
constexpr int MAX_BUNNIES = 32765;
constexpr float TEXT_AREA_HEIGHT = 30.0f;   // Reserve space at bottom for text
constexpr float GROUND_Y = -DISPLAY_HEIGHT / 2.0f + TEXT_AREA_HEIGHT;
constexpr float TOP_Y = DISPLAY_HEIGHT / 2.0f;
constexpr float ANIM_DURATION_MIN = 1.6f;   // Minimum animation duration
constexpr float ANIM_DURATION_MAX = 2.4f;   // Maximum animation duration
constexpr float ANIM_DELAY_MAX = 0.88f;      // Maximum random delay before animation starts

// Bunny structure
struct Bunny {
	int image_index;
	v2 position;
	float anim_time;      // Current animation progress (0.0 to 1.0)
	float anim_duration;  // Duration of one ping-pong cycle
	float delay;          // Delay before animation starts
};

// Global state
static CF_Sprite bunny_sprites[NUM_BUNNY_TYPES];
static dyna Bunny* bunnies = NULL;
static CF_Rnd rnd;

static const char* bunny_files[NUM_BUNNY_TYPES] = {
	"bunnymark_data/rabbitv3.png",
	"bunnymark_data/rabbitv3_ash.png",
	"bunnymark_data/rabbitv3_batman.png",
	"bunnymark_data/rabbitv3_bb8.png",
	"bunnymark_data/rabbitv3_frankenstein.png",
	"bunnymark_data/rabbitv3_neo.png",
	"bunnymark_data/rabbitv3_sonic.png",
	"bunnymark_data/rabbitv3_spidey.png",
	"bunnymark_data/rabbitv3_stormtrooper.png",
	"bunnymark_data/rabbitv3_superman.png",
	"bunnymark_data/rabbitv3_tron.png",
	"bunnymark_data/rabbitv3_wolverine.png",
};

static void spawn_bunnies(int count)
{
	float half_width = DISPLAY_WIDTH / 2.0f;

	for (int i = 0; i < count; i++) {
		Bunny bunny;
		bunny.image_index = cf_rnd_range_int(&rnd, 0, NUM_BUNNY_TYPES - 1);
		// Spawn at random X position across the screen
		bunny.position.x = cf_rnd_range_float(&rnd, -half_width, half_width);
		bunny.position.y = TOP_Y; // Start at top (like Defold)
		// Random animation duration so bunnies drift out of phase
		bunny.anim_duration = cf_rnd_range_float(&rnd, ANIM_DURATION_MIN, ANIM_DURATION_MAX);
		bunny.anim_time = 0.0f;
		bunny.delay = cf_rnd_range_float(&rnd, 0.0f, ANIM_DELAY_MAX);
		apush(bunnies, bunny);
	}
}

static void update_bunnies(float dt)
{
	int count = acount(bunnies);
	for (int i = 0; i < count; i++) {
		Bunny* b = &bunnies[i];

		// Wait for delay to expire before animating
		if (b->delay > 0.0f) {
			b->delay -= dt;
			continue;
		}

		// Advance animation time
		b->anim_time += dt / b->anim_duration;
		if (b->anim_time > 1.0f) {
			b->anim_time -= 1.0f;
		}

		// Ping-pong: convert 0→1 to 0→1→0
		float t = b->anim_time < 0.5f ? b->anim_time * 2.0f : 2.0f - b->anim_time * 2.0f;

		// Apply quadratic ease-in (like Defold's EASING_INQUAD - slow start, accelerate)
		float eased_t = cf_quad_in(t);

		// Interpolate Y position from top to ground (like Defold)
		b->position.y = cf_lerp(TOP_Y, GROUND_Y, eased_t);
	}
}

static void draw_bunnies()
{
	int count = acount(bunnies);
	for (int i = 0; i < count; i++) {
		Bunny* b = &bunnies[i];
		draw_push();
		draw_translate(b->position.x, b->position.y);
		draw_sprite(&bunny_sprites[b->image_index]);
		draw_pop();
	}
}

int main(int argc, char* argv[])
{
	CF_Result result = make_app(
		"Bunnymark",
		0, 0, 0,
		DISPLAY_WIDTH, DISPLAY_HEIGHT,
		CF_APP_OPTIONS_WINDOW_POS_CENTERED_BIT | CF_APP_OPTIONS_RESIZABLE_BIT,
		argv[0]
	);
	if (is_error(result)) {
		return -1;
	}

	// Initialize random number generator with better entropy
	uint64_t seed = (uint64_t)time(NULL);
	seed ^= (uint64_t)(uintptr_t)&rnd;  // Mix in address for more entropy
	seed ^= seed << 21;
	seed ^= seed >> 35;
	seed ^= seed << 4;
	rnd = rnd_seed(seed);

	// Load bunny sprites
	for (int i = 0; i < NUM_BUNNY_TYPES; i++) {
		bunny_sprites[i] = cf_make_easy_sprite_from_png(bunny_files[i], NULL);
	}

	// Pre-allocate array capacity to avoid reallocations
	afit(bunnies, MAX_BUNNIES);

	// Spawn initial bunnies
	spawn_bunnies(SPAWN_COUNT);

	// Increase atlas size for better batching with many sprites
	draw_set_atlas_dimensions(4096, 4096);

	// Set a white background like the original
	clear_color(1.0f, 1.0f, 1.0f, 1.0f);

	while (app_is_running()) {
		app_update();

		// Spawn more bunnies on mouse click
		if (mouse_just_pressed(CF_MOUSE_BUTTON_LEFT)) {
			spawn_bunnies(SPAWN_COUNT);
		}

		// Update bunny animations
		update_bunnies(CF_DELTA_TIME);

		// Draw all bunnies
		draw_bunnies();

		// Draw UI text at bottom center
		push_font_size(16);
		draw_push_color(color_black());

		char status_text[128];
		snprintf(status_text, sizeof(status_text), "Bunnies: %d FPS: %.2f. Click to add more", acount(bunnies), app_get_smoothed_framerate());
		v2 text_size_val = text_size(status_text);
		draw_text(status_text, V2(-text_size_val.x / 2.0f, -DISPLAY_HEIGHT / 2.0f + 16));

		draw_pop_color();
		pop_font_size();

		app_draw_onto_screen(true);
	}

	// Cleanup
	afree(bunnies);
	for (int i = 0; i < NUM_BUNNY_TYPES; i++) {
		cf_easy_sprite_unload(&bunny_sprites[i]);
	}
	destroy_app();

	return 0;
}
