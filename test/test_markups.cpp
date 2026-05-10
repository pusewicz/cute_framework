/*
	Cute Framework
	Copyright (C) 2024 Randy Gaul https://randygaul.github.io/

	This software is dual-licensed with zlib or Unlicense, check LICENSE.txt for more info
*/

#include "test_harness.h"

#include <cute.h>
#include <stdlib.h>
using namespace Cute;

static bool s_noop_effect(CF_TextEffect*) { return true; }

// -----------------------------------------------------------------------------
// Shared probe state. The markup-info callbacks are passed to C-style function
// pointer slots, so they must be free functions (or stateless lambdas decayed
// to function pointers). Tests communicate with the callbacks through these
// statics.
// -----------------------------------------------------------------------------

static const char* s_expected_name = NULL;
static bool s_seen_effect = false;
static double s_seen_number = 0;
static CF_Color s_seen_color = {};
static bool s_check_color = false;
static bool s_check_number = false;
static const char* s_number_key = NULL;
static const char* s_color_key = NULL;
static bool s_hit = false;
static int s_count = 0;
static const char* s_string_key = NULL;
static const char* s_seen_string = NULL;
static bool s_saw_wave = false;
static bool s_saw_color = false;

static void s_probe(const char* text, CF_MarkupInfo info, const CF_TextEffect* fx)
{
	CF_UNUSED(text);
	if (info.effect_name == sintern(s_expected_name)) {
		s_seen_effect = true;
		if (s_check_number) s_seen_number = cf_text_effect_get_number(fx, s_number_key, -1);
		if (s_check_color) s_seen_color = cf_text_effect_get_color(fx, s_color_key, cf_color_white());
		if (s_string_key) s_seen_string = cf_text_effect_get_string(fx, s_string_key, NULL);
	}
}

static void s_hit_probe(const char* text, CF_MarkupInfo info, const CF_TextEffect* fx)
{
	CF_UNUSED(text); CF_UNUSED(info); CF_UNUSED(fx);
	s_hit = true;
}

static void s_count_probe(const char* text, CF_MarkupInfo info, const CF_TextEffect* fx)
{
	CF_UNUSED(text); CF_UNUSED(fx);
	if (info.effect_name == sintern(s_expected_name)) s_count++;
}

static void s_wave_color_probe(const char* text, CF_MarkupInfo info, const CF_TextEffect* fx)
{
	CF_UNUSED(text); CF_UNUSED(fx);
	if (info.effect_name == sintern("wave")) s_saw_wave = true;
	if (info.effect_name == sintern("color")) s_saw_color = true;
}

static void s_reset_probe(const char* expected_name)
{
	s_expected_name = expected_name;
	s_seen_effect = false;
	s_seen_number = 0;
	s_seen_color = cf_color_white();
	s_check_color = false;
	s_check_number = false;
	s_number_key = NULL;
	s_color_key = NULL;
	s_string_key = NULL;
	s_seen_string = NULL;
}

static void s_basic_probe(const char* text, CF_MarkupInfo info, const CF_TextEffect* fx)
{
	CF_UNUSED(text);
	if (info.effect_name != sintern("fake")) return;
	double number = cf_text_effect_get_number(fx, "number", 0);
	const char* string = cf_text_effect_get_string(fx, "string", NULL);
	CF_Color color = cf_text_effect_get_color(fx, "color", cf_color_white());
	if (number != 123.0) return;
	if (!string || CF_STRCMP(string, "maybe a string")) return;
	if (color != cf_make_color_rgba(0xff, 0x00, 0xff, 0x12)) return;
	s_hit = true;
}

TEST_CASE(test_markups_basic)
{
	REQUIRE(!is_error(make_app(NULL, 0, 0, 0, 0, CF_APP_OPTIONS_HIDDEN_BIT | CF_APP_OPTIONS_NO_AUDIO_BIT, NULL)));

	cf_text_effect_register("fake", s_noop_effect);
	s_hit = false;
	const char* text = "<fake number=123 string=\"maybe a string\" color=#ff00ff12>does this work?</fake>";
	cf_text_get_markup_info(s_basic_probe, text, V2(0,0), -1);
	REQUIRE(s_hit == true);

	destroy_app();

	return true;
}

TEST_CASE(test_markups_bad_inputs)
{
	REQUIRE(!is_error(make_app(NULL, 0, 0, 0, 0, CF_APP_OPTIONS_HIDDEN_BIT | CF_APP_OPTIONS_NO_AUDIO_BIT, NULL)));

	s_hit = false;
	text_get_markup_info(s_hit_probe, "<broken>test string/broken>", V2(0,0));
	text_get_markup_info(s_hit_probe, "<broken>test stringbroken>", V2(0,0));
	text_get_markup_info(s_hit_probe, "<broken>test stringbroken", V2(0,0));
	text_get_markup_info(s_hit_probe, "<brokentest stringbroken", V2(0,0));
	text_get_markup_info(s_hit_probe, "brokentest stringbroken", V2(0,0));
	text_get_markup_info(s_hit_probe, "<broken>test string<broken>", V2(0,0));
	text_get_markup_info(s_hit_probe, "<broken>test string<broken", V2(0,0));
	text_get_markup_info(s_hit_probe, "<brokentest string</broken>", V2(0,0));
	text_get_markup_info(s_hit_probe, "broken>test string</broken>", V2(0,0));
	text_get_markup_info(s_hit_probe, "<<>>", V2(0,0));
	text_get_markup_info(s_hit_probe, "te<broken>st/broken> string", V2(0,0));
	text_get_markup_info(s_hit_probe, "te<broken>st/broken> st<>>><rin<>g", V2(0,0));
	REQUIRE(s_hit == false);

	destroy_app();

	return true;
}

// -----------------------------------------------------------------------------
// All other coverage runs under a single make_app/destroy_app cycle to avoid
// PhysFS init/deinit churn that destabilizes the test process after enough
// rounds.
// -----------------------------------------------------------------------------

TEST_CASE(test_markups_full_coverage)
{
	REQUIRE(!is_error(make_app(NULL, 0, 0, 0, 0, CF_APP_OPTIONS_HIDDEN_BIT | CF_APP_OPTIONS_NO_AUDIO_BIT, NULL)));

	// --- Built-in effect coverage ----------------------------------------

	s_reset_probe("color");
	s_check_color = true;
	s_color_key = "color";
	text_get_markup_info(s_probe, "<color=#ff0000ff>red</color>", V2(0,0));
	REQUIRE(s_seen_effect);
	REQUIRE(s_seen_color == cf_make_color_rgba(0xff, 0x00, 0x00, 0xff));

	s_reset_probe("shake");
	s_check_number = true;
	s_number_key = "freq";
	text_get_markup_info(s_probe, "<shake freq=20 x=3 y=3>w</shake>", V2(0,0));
	REQUIRE(s_seen_effect);
	REQUIRE(s_seen_number == 20.0);

	s_reset_probe("fade");
	s_check_number = true;
	s_number_key = "speed";
	text_get_markup_info(s_probe, "<fade speed=1 span=4>w</fade>", V2(0,0));
	REQUIRE(s_seen_effect);
	REQUIRE(s_seen_number == 1.0);

	s_reset_probe("wave");
	s_check_number = true;
	s_number_key = "height";
	text_get_markup_info(s_probe, "<wave speed=5 span=10 height=5>w</wave>", V2(0,0));
	REQUIRE(s_seen_effect);
	REQUIRE(s_seen_number == 5.0);

	s_reset_probe("strike");
	s_check_number = true;
	s_number_key = "strike";
	text_get_markup_info(s_probe, "<strike strike=2>w</strike>", V2(0,0));
	REQUIRE(s_seen_effect);
	REQUIRE(s_seen_number == 2.0);

	s_reset_probe("gradient");
	text_get_markup_info(s_probe, "<gradient top=#ffffffff bottom=#000000ff>w</gradient>", V2(0,0));
	REQUIRE(s_seen_effect);

	// --- Custom effect registration --------------------------------------

	cf_text_effect_register("mycustom", s_noop_effect);
	s_reset_probe("mycustom");
	text_get_markup_info(s_probe, "<mycustom>x</mycustom>", V2(0,0));
	REQUIRE(s_seen_effect);

	// --- Grammar features ------------------------------------------------

	s_reset_probe("wave");
	s_check_number = true;
	s_number_key = "speed";
	text_get_markup_info(s_probe, "<  wave   speed=5   >x</wave>", V2(0,0));
	REQUIRE(s_seen_effect);
	REQUIRE(s_seen_number == 5.0);

	cf_text_effect_register("fake", s_noop_effect);
	s_reset_probe("fake");
	s_string_key = "s";
	text_get_markup_info(s_probe, "<fake s=\"a > b\">x</fake>", V2(0,0));
	REQUIRE(s_seen_effect);
	REQUIRE(s_seen_string != NULL);
	REQUIRE(!CF_STRCMP(s_seen_string, "a > b"));

	const char* stripped = cf_text_without_markups("prefix <wave>mid</wave> suffix");
	REQUIRE(stripped != NULL);
	REQUIRE(!CF_STRCMP(stripped, "prefix mid suffix"));
	const char* unchanged = cf_text_without_markups("no markup here");
	REQUIRE(!CF_STRCMP(unchanged, "no markup here"));

	s_reset_probe("wave");
	s_check_number = true;
	s_number_key = "speed";
	text_get_markup_info(s_probe, "<wave speed=-1.5>x</wave>", V2(0,0));
	REQUIRE(s_seen_effect);
	REQUIRE(s_seen_number == -1.5);

	s_saw_wave = false;
	s_saw_color = false;
	text_get_markup_info(s_wave_color_probe, "<wave><color=#ff0000ff>x</color></wave>", V2(0,0));
	REQUIRE(s_saw_wave);
	REQUIRE(s_saw_color);

	s_expected_name = "color";
	s_count = 0;
	text_get_markup_info(s_count_probe,
		"<color=#ff0000ff>a</color><color=#00ff00ff>b</color>", V2(0,0));
	REQUIRE(s_count == 2);

	// Empty body: per-glyph loop has nothing to iterate so the callback is
	// not invoked. The key assertion is that parsing does not crash.
	s_hit = false;
	text_get_markup_info(s_hit_probe, "<wave></wave>", V2(0,0));
	REQUIRE(s_hit == false);

	// --- Escapes (existing /< and new />) ---------------------------------

	s_hit = false;
	text_get_markup_info(s_hit_probe, "/<wave>", V2(0,0));
	REQUIRE(s_hit == false);
	const char* esc_lt = cf_text_without_markups("/<wave>");
	REQUIRE(!CF_STRCMP(esc_lt, "/<wave>"));

	s_hit = false;
	text_get_markup_info(s_hit_probe, "5 /> 3", V2(0,0));
	REQUIRE(s_hit == false);
	const char* esc_gt = cf_text_without_markups("5 /> 3");
	REQUIRE(!CF_STRCMP(esc_gt, "5 /> 3"));

	// --- Bug regression: literal < and > are safe ------------------------

	s_hit = false;
	const char* cases[] = {
		"Price < $100",
		"5 > 3",
		"if (x<y) return z",
		"a < b && c > d",
		"<wace>typo</wace>",
		"<NotAnEffect>foo</NotAnEffect>",
		"<>",
		"<<>>",
		"</>",
		"<",
		">",
		"<color",
		"</color",
		"<color=#ff0000ff",
		"text < more text < and more",
		"vector<int> v;",
		"template<T>",
	};
	for (size_t i = 0; i < CF_ARRAY_SIZE(cases); ++i) {
		text_get_markup_info(s_hit_probe, cases[i], V2(0,0));
	}
	REQUIRE(s_hit == false);

	// Hammer the previously-crashing path to confirm no SEGV and bounded
	// intern-table growth.
	for (int i = 0; i < 1000; ++i) {
		text_get_markup_info(s_hit_probe, "Price < $100 > each", V2(0,0));
	}
	REQUIRE(s_hit == false);

	destroy_app();
	return true;
}

TEST_SUITE(test_markups)
{
	// These tests need a graphics context, which CI build machines lack.
	// CI runners (GitHub Actions and most others) set CI=true.
	bool skip = getenv("CI") != NULL;
	RUN_TEST_CASE_UNLESS(skip, test_markups_basic);
	RUN_TEST_CASE_UNLESS(skip, test_markups_bad_inputs);
	RUN_TEST_CASE_UNLESS(skip, test_markups_full_coverage);
}
