/*
	Cute Framework
	Copyright (C) 2026 Randy Gaul https://randygaul.github.io/

	This software is dual-licensed with zlib or Unlicense, check LICENSE.txt for more info
*/

#include "test_harness.h"
#include "test_app_shared.h"

#include <cute.h>
#include <SDL3/SDL.h>
using namespace Cute;

// A failed REQUIRE returns out of the test case early, so the app is destroyed via RAII, or the
// leaked app would break cf_make_app in whichever test case runs next. Named apart from the other
// test files' guards, as same-named structs with different inline bodies are an ODR violation.
struct TouchAppGuard
{
	~TouchAppGuard() { cf_destroy_app(); }
};

static void s_push_finger(SDL_EventType type, uint64_t id, float x, float y)
{
	SDL_Event event = {};
	event.type = type;
	event.tfinger.fingerID = (SDL_FingerID)id;
	event.tfinger.x = x;
	event.tfinger.y = y;
	event.tfinger.pressure = 1.0f;
	cf_app_push_event(&event);
}

static bool s_make_headless_app()
{
	// A shared GPU app left alive by an earlier suite would make cf_make_app fail.
	test_shutdown_shared_app();
	return !cf_is_error(cf_make_app(NULL, 0, 0, 0, 200, 100, CF_APP_OPTIONS_HIDDEN_BIT | CF_APP_OPTIONS_NO_GFX_BIT | CF_APP_OPTIONS_NO_AUDIO_BIT, NULL));
}

// A tap that starts and ends between two updates is gone from cf_touch_get_all by the time the game
// looks, but its press must still be reported, at the position the finger landed.
TEST_CASE(test_touch_pressed_reports_a_tap_lifted_within_the_frame)
{
	REQUIRE(s_make_headless_app());
	TouchAppGuard guard;

	s_push_finger(SDL_EVENT_FINGER_DOWN, 7, 0.25f, 0.75f);
	s_push_finger(SDL_EVENT_FINGER_UP, 7, 0.30f, 0.80f);
	cf_app_update(NULL);

	CF_Touch* live = NULL;
	REQUIRE(cf_touch_get_all(&live) == 0);

	CF_Touch* pressed = NULL;
	REQUIRE(cf_touch_get_pressed(&pressed) == 1);
	REQUIRE(pressed[0].id == 7);
	REQUIRE(pressed[0].x == 0.25f);
	REQUIRE(pressed[0].y == 0.75f);
	return true;
}

// The list is one frame long: a finger that is still down stays in cf_touch_get_all, but is only
// "pressed" on the frame it landed.
TEST_CASE(test_touch_pressed_lasts_one_frame)
{
	REQUIRE(s_make_headless_app());
	TouchAppGuard guard;

	s_push_finger(SDL_EVENT_FINGER_DOWN, 1, 0.1f, 0.2f);
	s_push_finger(SDL_EVENT_FINGER_DOWN, 2, 0.6f, 0.7f);
	cf_app_update(NULL);

	CF_Touch* pressed = NULL;
	REQUIRE(cf_touch_get_pressed(&pressed) == 2);
	REQUIRE(pressed[0].id == 1);
	REQUIRE(pressed[1].id == 2);
	CF_Touch* live = NULL;
	REQUIRE(cf_touch_get_all(&live) == 2);

	// Finger 1 drags: motion moves the live touch, and is not a new press.
	s_push_finger(SDL_EVENT_FINGER_MOTION, 1, 0.4f, 0.5f);
	cf_app_update(NULL);
	REQUIRE(cf_touch_get_pressed(&pressed) == 0);
	REQUIRE(cf_touch_get_all(&live) == 2);
	CF_Touch moved;
	REQUIRE(cf_touch_get(1, &moved));
	REQUIRE(moved.x == 0.4f);
	REQUIRE(moved.y == 0.5f);

	// A second frame with no events reports nothing new either.
	cf_app_update(NULL);
	REQUIRE(cf_touch_get_pressed(&pressed) == 0);
	REQUIRE(cf_touch_get_all(&live) == 2);

	// A lifted finger leaves cf_touch_get_all and a fresh touch with the same id is a new press.
	s_push_finger(SDL_EVENT_FINGER_UP, 1, 0.4f, 0.5f);
	s_push_finger(SDL_EVENT_FINGER_DOWN, 1, 0.9f, 0.9f);
	cf_app_update(NULL);
	REQUIRE(cf_touch_get_pressed(&pressed) == 1);
	REQUIRE(pressed[0].x == 0.9f);
	REQUIRE(cf_touch_get_all(&live) == 2);
	return true;
}

TEST_SUITE(test_input)
{
	RUN_TEST_CASE(test_touch_pressed_reports_a_tap_lifted_within_the_frame);
	RUN_TEST_CASE(test_touch_pressed_lasts_one_frame);
}
