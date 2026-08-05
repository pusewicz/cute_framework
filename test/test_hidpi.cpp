/*
	Cute Framework
	Copyright (C) 2026 Randy Gaul https://randygaul.github.io/

	This software is dual-licensed with zlib or Unlicense, check LICENSE.txt for more info
*/

#include "test_harness.h"
#include "test_app_shared.h"

#include <cute.h>
#include <internal/cute_app_internal.h>

using namespace Cute;

// On a HiDPI (Retina) display the default app canvas is logical_size * pixel_scale physical
// pixels while the public draw API stays in logical points. CI and most dev machines run at
// pixel_scale 1.0 where points and pixels are the same number, so a points/pixels mixup is
// invisible there -- these tests force a non-unity pixel_scale through the same code path a
// real display-density change takes, making the HiDPI contract testable everywhere.
//
// Regression context: 3f5a8283 set the default 2d projection to the canvas's *pixel*
// dimensions, which halved everything drawn in point space on 2x displays.

#define LOGICAL_W 320
#define LOGICAL_H 240

static bool s_px_near(CF_Pixel p, int r, int g, int b, int a, int tol)
{
	auto absi = [](int v) { return v < 0 ? -v : v; };
	bool ok = absi((int)p.colors.r - r) <= tol && absi((int)p.colors.g - g) <= tol && absi((int)p.colors.b - b) <= tol && absi((int)p.colors.a - a) <= tol;
	if (!ok) printf("pixel (%d %d %d %d) expected (%d %d %d %d) +/-%d\n", p.colors.r, p.colors.g, p.colors.b, p.colors.a, r, g, b, a, tol);
	return ok;
}

// A failed REQUIRE returns out of the test case early; RAII keeps the forced pixel_scale
// from leaking into whichever test runs next (see also test_app_shared's own sweep).
struct HidpiGuard
{
	~HidpiGuard()
	{
		cf_app_force_pixel_scale(0);
		test_destroy_app();
	}
};

static bool s_readback_canvas(CF_Canvas canvas, int w, int h, CF_Pixel* out)
{
	CF_Readback rb = cf_canvas_readback(canvas);
	REQUIRE(rb.id);
	while (!cf_readback_ready(rb)) {}
	int size = w * h * (int)sizeof(CF_Pixel);
	REQUIRE(cf_readback_size(rb) == size);
	cf_readback_data(rb, out, size);
	cf_destroy_readback(rb);
	return true;
}

// Rect scaling must round EDGES, not x/w independently: at fractional pixel densities
// (Wayland 1.25/1.5, iOS ~2.6) independent rounding lets an in-bounds logical rect scale
// one pixel past the canvas edge (an API-validation failure for scissors on Metal), and
// abutting logical rects drift apart by a pixel. Pure math -- no GPU needed.
TEST_CASE(test_hidpi_scale_rect_rounds_edges)
{
	// Logical {1, 0, 319, 240} inside a 320x240 window at 1.5x (canvas 480x360): x and w
	// both round up independently (1.5 -> 2, 478.5 -> 479, sum 481 > 480). Edge rounding
	// keeps x + w == round(320 * 1.5) == 480.
	CF_Rect r = cf_draw_scale_rect({ 1, 0, 319, 240 }, 1.5f, 1.5f);
	REQUIRE(r.x + r.w <= 480);
	REQUIRE(r.x + r.w == 480); // Right edge lands exactly on the canvas edge.
	REQUIRE(r.y + r.h == 360);

	// Abutting logical rects stay seamless after scaling: shared edge maps identically.
	CF_Rect a = cf_draw_scale_rect({ 0, 0, 213, 240 }, 1.5f, 1.5f);
	CF_Rect b = cf_draw_scale_rect({ 213, 0, 107, 240 }, 1.5f, 1.5f);
	REQUIRE(a.x + a.w == b.x);

	// Integer scales are exact either way.
	CF_Rect c = cf_draw_scale_rect({ 80, 60, 160, 120 }, 2.0f, 2.0f);
	REQUIRE(c.x == 160 && c.y == 120 && c.w == 320 && c.h == 240);

	// Per-axis scales (a one-shot cf_app_set_canvas_size override with a different aspect
	// than the window): 320x240 points onto a 300x200 canvas.
	CF_Rect d = cf_draw_scale_rect({ 80, 60, 160, 120 }, 300.0f / 320.0f, 200.0f / 240.0f);
	REQUIRE(d.x == 75 && d.y == 50 && d.w == 150 && d.h == 100);
	return true;
}

// The force hook itself: forcing 2x must recreate the default canvas at 2x the logical
// window size (the same effect as a real display-density change), and forcing 0 must
// restore the true SDL-reported density.
TEST_CASE(test_hidpi_forced_scale_resizes_canvas)
{
	if (!test_make_app(LOGICAL_W, LOGICAL_H)) return true; // Headless CI: no display/GPU.
	HidpiGuard guard;

	cf_app_force_pixel_scale(2.0f);
	REQUIRE(cf_app_get_pixel_scale() == 2.0f);
	REQUIRE(cf_app_get_canvas_width() == LOGICAL_W * 2);
	REQUIRE(cf_app_get_canvas_height() == LOGICAL_H * 2);
	// The logical window size is unchanged -- only the rasterization resolution moved.
	REQUIRE(cf_app_get_width() == LOGICAL_W);
	REQUIRE(cf_app_get_height() == LOGICAL_H);

	cf_app_force_pixel_scale(0);
	float real = cf_app_get_pixel_scale();
	REQUIRE(real > 0);
	REQUIRE(cf_app_get_canvas_width() == (int)CF_ROUNDF(LOGICAL_W * real));
	return true;
}

// CF_APP_OPTIONS_NO_HIGH_DPI_BIT documents pixel_scale as pinned at 1.0 -- a forced value
// must not manufacture a state production can never reach (the real density path no-ops
// under this bit), or a HiDPI test against a NO_HIGH_DPI app would validate fiction.
TEST_CASE(test_hidpi_force_respects_no_high_dpi)
{
	if (!test_make_app(LOGICAL_W, LOGICAL_H, CF_APP_OPTIONS_NO_HIGH_DPI_BIT)) return true; // Headless CI: no display/GPU.
	HidpiGuard guard;

	cf_app_force_pixel_scale(2.0f);
	REQUIRE(cf_app_get_pixel_scale() == 1.0f);
	REQUIRE(cf_app_get_canvas_width() == LOGICAL_W);
	REQUIRE(cf_app_get_canvas_height() == LOGICAL_H);
	return true;
}

// NO_GFX apps never attach a graphics backend or size a canvas (cf_make_app only does so
// under use_gfx), so the force hook must update pixel_scale without touching the canvas --
// recreating one would dispatch into a NULL backend.
TEST_CASE(test_hidpi_force_is_safe_without_gfx)
{
	if (!test_make_app(LOGICAL_W, LOGICAL_H, CF_APP_OPTIONS_NO_GFX_BIT)) return true;
	HidpiGuard guard;

	cf_app_force_pixel_scale(2.0f);
	REQUIRE(cf_app_get_pixel_scale() == 2.0f);
	return true;
}

// The default 2d projection spans logical points, so world coordinates land on the same
// spot of a 1:1 user canvas no matter the display density. At the broken pixel-space
// projection everything shrinks toward the center and the probe reads background.
TEST_CASE(test_hidpi_default_projection_is_points)
{
	if (!test_make_app(LOGICAL_W, LOGICAL_H)) return true; // Headless CI: no display/GPU.
	HidpiGuard guard;

	cf_app_force_pixel_scale(2.0f);
	// One empty frame: reset_cam at end of frame latches the DEFAULT projection into mvp,
	// so this test observes the projection itself rather than whatever matrix an earlier
	// test (or cf_make_draw on a fresh app) happened to leave latched.
	cf_app_update(NULL);
	cf_app_draw_onto_screen(false);
	cf_app_update(NULL);

	int w = LOGICAL_W, h = LOGICAL_H;
	CF_Canvas canvas = cf_make_canvas(cf_canvas_defaults(w, h));
	CF_Pixel* px = (CF_Pixel*)cf_alloc(w * h * (int)sizeof(CF_Pixel));

	// A bar left of center, spanning y=0 so the probe row is insensitive to readback
	// row order: world x in [-140, -60].
	cf_draw_push_color(cf_make_color_rgb_f(1.0f, 0, 0));
	cf_draw_quad_fill(cf_make_aabb(cf_v2(-140, -20), cf_v2(-60, 20)), 0);
	cf_draw_pop_color();
	cf_render_to(canvas, true);
	cf_app_draw_onto_screen(false);

	bool ok = s_readback_canvas(canvas, w, h, px);
	REQUIRE(ok);
	// World (-100, 0) is pixel column w/2 - 100 when one world unit is one point.
	REQUIRE(s_px_near(px[(h / 2) * w + (w / 2 - 100)], 255, 0, 0, 255, 3));
	// Just outside the bar: untouched.
	REQUIRE(s_px_near(px[(h / 2) * w + (w / 2 - 150)], 0, 0, 0, 0, 0));

	cf_free(px);
	cf_destroy_canvas(canvas);
	return true;
}

// Through the real present path: a quad spanning the exact logical extent must cover every
// pixel of the (2x larger) default canvas. At the broken projection it covers only the
// central quarter and the corners read background.
TEST_CASE(test_hidpi_full_extent_covers_app_canvas)
{
	if (!test_make_app(LOGICAL_W, LOGICAL_H)) return true; // Headless CI: no display/GPU.
	HidpiGuard guard;

	cf_app_force_pixel_scale(2.0f);
	// Latch the default projection into mvp (see test_hidpi_default_projection_is_points).
	cf_app_update(NULL);
	cf_app_draw_onto_screen(false);
	cf_app_update(NULL);

	int w = cf_app_get_canvas_width();
	int h = cf_app_get_canvas_height();
	REQUIRE(w == LOGICAL_W * 2 && h == LOGICAL_H * 2);
	CF_Pixel* px = (CF_Pixel*)cf_alloc(w * h * (int)sizeof(CF_Pixel));

	cf_draw_push_color(cf_make_color_rgb_f(1.0f, 0, 0));
	cf_draw_quad_fill(cf_make_aabb(cf_v2(-LOGICAL_W / 2.0f, -LOGICAL_H / 2.0f), cf_v2(LOGICAL_W / 2.0f, LOGICAL_H / 2.0f)), 0);
	cf_draw_pop_color();
	cf_app_draw_onto_screen(true); // Renders the remaining draw commands onto the (cleared) app canvas.

	bool ok = s_readback_canvas(cf_app_get_canvas(), w, h, px);
	REQUIRE(ok);
	// Probe 5px inside each corner (clear of the AA band) plus the center.
	REQUIRE(s_px_near(px[5 * w + 5], 255, 0, 0, 255, 3));
	REQUIRE(s_px_near(px[5 * w + (w - 6)], 255, 0, 0, 255, 3));
	REQUIRE(s_px_near(px[(h - 6) * w + 5], 255, 0, 0, 255, 3));
	REQUIRE(s_px_near(px[(h - 6) * w + (w - 6)], 255, 0, 0, 255, 3));
	REQUIRE(s_px_near(px[(h / 2) * w + (w / 2)], 255, 0, 0, 255, 3));

	cf_free(px);
	return true;
}

// cf_app_set_size recreates the canvas and refreshes the projection immediately -- and the
// very next frame must draw with it. The projection's companion mvp is only re-latched at
// end of frame by reset_cam, so a projection refresh that skips mvp renders the first
// post-resize frame with the stale matrix.
TEST_CASE(test_hidpi_first_frame_after_resize)
{
	if (!test_make_app(LOGICAL_W, LOGICAL_H)) return true; // Headless CI: no display/GPU.
	HidpiGuard guard;

	cf_app_force_pixel_scale(2.0f);
	cf_app_update(NULL);

	cf_app_set_size(400, 300);
	int w = cf_app_get_canvas_width();
	int h = cf_app_get_canvas_height();
	REQUIRE(w == 800 && h == 600);
	CF_Pixel* px = (CF_Pixel*)cf_alloc(w * h * (int)sizeof(CF_Pixel));

	// Draw WITHOUT an intervening cf_app_update: this is the same frame the resize
	// happened on, exactly where a stale mvp would bite.
	cf_draw_push_color(cf_make_color_rgb_f(0, 1.0f, 0));
	cf_draw_quad_fill(cf_make_aabb(cf_v2(-200, -150), cf_v2(200, 150)), 0);
	cf_draw_pop_color();
	cf_app_draw_onto_screen(true);

	bool ok = s_readback_canvas(cf_app_get_canvas(), w, h, px);
	REQUIRE(ok);
	REQUIRE(s_px_near(px[5 * w + 5], 0, 255, 0, 255, 3));
	REQUIRE(s_px_near(px[(h - 6) * w + (w - 6)], 0, 255, 0, 255, 3));
	REQUIRE(s_px_near(px[(h / 2) * w + (w / 2)], 0, 255, 0, 255, 3));

	cf_free(px);
	return true;
}

// cf_app_set_canvas_size is a one-shot override: the canvas takes an exact pixel size
// until the next recreation event snaps it back. Draw coordinates STAY in window points
// throughout -- the projection maps them onto whatever canvas is current, and viewport/
// scissor rects follow the same canvas/window ratio the geometry does.
TEST_CASE(test_hidpi_one_shot_canvas_keeps_points_projection)
{
	if (!test_make_app(LOGICAL_W, LOGICAL_H)) return true; // Headless CI: no display/GPU.
	HidpiGuard guard;

	cf_app_force_pixel_scale(2.0f);
	// One-shot: exactly 300x200 pixels, no density scaling. Distinct from the logical
	// window size, 2x of it, and the window's aspect, so the probes can tell them apart.
	cf_app_set_canvas_size(300, 200);
	REQUIRE(cf_app_get_canvas_width() == 300);
	REQUIRE(cf_app_get_canvas_height() == 200);
	// Latch the default projection into mvp (see test_hidpi_default_projection_is_points).
	cf_app_update(NULL);
	cf_app_draw_onto_screen(false);
	cf_app_update(NULL);

	int w = 300, h = 200;
	CF_Pixel* px = (CF_Pixel*)cf_alloc(w * h * (int)sizeof(CF_Pixel));

	// The full LOGICAL extent (window points) covers the whole override canvas.
	cf_draw_push_color(cf_make_color_rgb_f(0, 0, 1.0f));
	cf_draw_quad_fill(cf_make_aabb(cf_v2(-LOGICAL_W / 2.0f, -LOGICAL_H / 2.0f), cf_v2(LOGICAL_W / 2.0f, LOGICAL_H / 2.0f)), 0);
	cf_draw_pop_color();
	cf_app_draw_onto_screen(true);

	bool ok = s_readback_canvas(cf_app_get_canvas(), w, h, px);
	REQUIRE(ok);
	REQUIRE(s_px_near(px[5 * w + 5], 0, 0, 255, 255, 3));
	REQUIRE(s_px_near(px[(h - 6) * w + (w - 6)], 0, 0, 255, 255, 3));
	REQUIRE(s_px_near(px[(h / 2) * w + (w / 2)], 0, 0, 255, 255, 3));

	// A scissor in window points maps through the same canvas/window ratio as geometry:
	// {80, 60, 160, 120} of 320x240 lands on {75, 50, 150, 100} of the 300x200 canvas.
	cf_app_update(NULL);
	CF_Rect scissor = { LOGICAL_W / 4, LOGICAL_H / 4, LOGICAL_W / 2, LOGICAL_H / 2 };
	cf_draw_push_scissor(scissor);
	cf_draw_push_color(cf_make_color_rgb_f(1.0f, 1.0f, 0));
	cf_draw_quad_fill(cf_make_aabb(cf_v2(-LOGICAL_W / 2.0f, -LOGICAL_H / 2.0f), cf_v2(LOGICAL_W / 2.0f, LOGICAL_H / 2.0f)), 0);
	cf_draw_pop_color();
	cf_draw_pop_scissor();
	cf_app_draw_onto_screen(true);

	ok = s_readback_canvas(cf_app_get_canvas(), w, h, px);
	REQUIRE(ok);
	REQUIRE(s_px_near(px[(h / 2) * w + (w / 2)], 255, 255, 0, 255, 3)); // Inside the region.
	REQUIRE(s_px_near(px[25 * w + (w / 2)], 0, 0, 0, 0, 0));           // Above/below the region.
	REQUIRE(s_px_near(px[(h / 2) * w + 37], 0, 0, 0, 0, 0));           // Left of the region.

	// The next recreation event snaps the override back to window * pixel_scale.
	cf_app_set_size(LOGICAL_W, LOGICAL_H);
	REQUIRE(cf_app_get_canvas_width() == LOGICAL_W * 2);
	REQUIRE(cf_app_get_canvas_height() == LOGICAL_H * 2);

	cf_free(px);
	return true;
}

// cf_draw_push_scissor rects are in the same logical units as everything else drawn on the
// default app canvas, so on a 2x display they must scale to physical pixels along with the
// canvas. A scissor computed from cf_app_get_width() would otherwise clip a quarter-area
// region. The rect is centered so every probe is insensitive to readback row order, and the
// tiled path (which intersects the user scissor with its own coverage box) is exercised too.
TEST_CASE(test_hidpi_scissor_is_logical_on_app_canvas)
{
	if (!test_make_app(LOGICAL_W, LOGICAL_H)) return true; // Headless CI: no display/GPU.
	HidpiGuard guard;
	// Restores the engine's default auto tile routing even when a REQUIRE returns early --
	// leaking forced-instanced mode would silently drop tiled-path coverage for every
	// later test in the shared-app run.
	struct TiledModeGuard { ~TiledModeGuard() { cf_draw_set_tiled_auto(); } } tiled_guard;

	cf_app_force_pixel_scale(2.0f);
	// Latch the default projection into mvp (see test_hidpi_default_projection_is_points).
	cf_app_update(NULL);
	cf_app_draw_onto_screen(false);

	int w = cf_app_get_canvas_width();
	int h = cf_app_get_canvas_height();
	REQUIRE(w == LOGICAL_W * 2 && h == LOGICAL_H * 2);
	CF_Pixel* px = (CF_Pixel*)cf_alloc(w * h * (int)sizeof(CF_Pixel));

	for (int mode = 0; mode < 2; ++mode) {
		if (mode == 1 && !cf_draw_tiled_available()) break; // No SSBOs on GLES.
		cf_draw_set_tiled_enabled(mode == 1);
		cf_app_update(NULL);

		// Centered quarter of the window in points: {80, 60, 160, 120} of 320x240.
		CF_Rect scissor = { LOGICAL_W / 4, LOGICAL_H / 4, LOGICAL_W / 2, LOGICAL_H / 2 };
		cf_draw_push_scissor(scissor);
		cf_draw_push_color(cf_make_color_rgb_f(1.0f, 0, 0));
		cf_draw_quad_fill(cf_make_aabb(cf_v2(-LOGICAL_W / 2.0f, -LOGICAL_H / 2.0f), cf_v2(LOGICAL_W / 2.0f, LOGICAL_H / 2.0f)), 0);
		cf_draw_pop_color();
		cf_draw_pop_scissor();
		cf_app_draw_onto_screen(true);

		bool ok = s_readback_canvas(cf_app_get_canvas(), w, h, px);
		REQUIRE(ok);
		// Clipped region in pixels: {160, 120, 320, 240}, centered.
		REQUIRE(s_px_near(px[(h / 2) * w + (w / 2)], 255, 0, 0, 255, 3)); // Inside.
		REQUIRE(s_px_near(px[100 * w + (w / 2)], 0, 0, 0, 0, 0));        // Above/below the region.
		REQUIRE(s_px_near(px[(h / 2) * w + 100], 0, 0, 0, 0, 0));        // Left of the region.
	}

	cf_free(px);
	return true;
}

// Same contract for cf_draw_push_viewport: logical units on the app canvas. (A set viewport
// routes commands down the instanced path, so there is no tiled variant to cover.)
TEST_CASE(test_hidpi_viewport_is_logical_on_app_canvas)
{
	if (!test_make_app(LOGICAL_W, LOGICAL_H)) return true; // Headless CI: no display/GPU.
	HidpiGuard guard;

	cf_app_force_pixel_scale(2.0f);
	// Latch the default projection into mvp (see test_hidpi_default_projection_is_points).
	cf_app_update(NULL);
	cf_app_draw_onto_screen(false);
	cf_app_update(NULL);

	int w = cf_app_get_canvas_width();
	int h = cf_app_get_canvas_height();
	REQUIRE(w == LOGICAL_W * 2 && h == LOGICAL_H * 2);
	CF_Pixel* px = (CF_Pixel*)cf_alloc(w * h * (int)sizeof(CF_Pixel));

	CF_Rect viewport = { LOGICAL_W / 4, LOGICAL_H / 4, LOGICAL_W / 2, LOGICAL_H / 2 };
	cf_draw_push_viewport(viewport);
	cf_draw_push_color(cf_make_color_rgb_f(1.0f, 0, 0));
	cf_draw_quad_fill(cf_make_aabb(cf_v2(-LOGICAL_W / 2.0f, -LOGICAL_H / 2.0f), cf_v2(LOGICAL_W / 2.0f, LOGICAL_H / 2.0f)), 0);
	cf_draw_pop_color();
	cf_draw_pop_viewport();
	cf_app_draw_onto_screen(true);

	bool ok = s_readback_canvas(cf_app_get_canvas(), w, h, px);
	REQUIRE(ok);
	// The full-extent quad fills exactly the viewport: pixels {160, 120, 320, 240}, centered.
	REQUIRE(s_px_near(px[(h / 2) * w + (w / 2)], 255, 0, 0, 255, 3)); // Inside.
	REQUIRE(s_px_near(px[100 * w + (w / 2)], 0, 0, 0, 0, 0));        // Above/below the region.
	REQUIRE(s_px_near(px[(h / 2) * w + 100], 0, 0, 0, 0, 0));        // Left of the region.

	cf_free(px);
	return true;
}

// User canvases are sized by the user in pixels and are not density-scaled, so their
// viewport/scissor rects stay 1:1 no matter the display density. Guards that the app-canvas
// scaling above does not leak into user canvases.
TEST_CASE(test_hidpi_scissor_stays_raw_on_user_canvas)
{
	if (!test_make_app(LOGICAL_W, LOGICAL_H)) return true; // Headless CI: no display/GPU.
	HidpiGuard guard;

	cf_app_force_pixel_scale(2.0f);
	cf_app_update(NULL);
	cf_app_draw_onto_screen(false);
	cf_app_update(NULL);

	int w = 100, h = 100;
	CF_Canvas canvas = cf_make_canvas(cf_canvas_defaults(w, h));
	CF_Pixel* px = (CF_Pixel*)cf_alloc(w * h * (int)sizeof(CF_Pixel));

	CF_Rect scissor = { 25, 25, 50, 50 }; // Centered, in this canvas's own pixels.
	cf_draw_push_scissor(scissor);
	cf_draw_push_color(cf_make_color_rgb_f(0, 1.0f, 0));
	cf_draw_quad_fill(cf_make_aabb(cf_v2(-LOGICAL_W / 2.0f, -LOGICAL_H / 2.0f), cf_v2(LOGICAL_W / 2.0f, LOGICAL_H / 2.0f)), 0);
	cf_draw_pop_color();
	cf_draw_pop_scissor();
	cf_render_to(canvas, true);
	cf_app_draw_onto_screen(false);

	bool ok = s_readback_canvas(canvas, w, h, px);
	REQUIRE(ok);
	REQUIRE(s_px_near(px[(h / 2) * w + (w / 2)], 0, 255, 0, 255, 3)); // Inside.
	REQUIRE(s_px_near(px[10 * w + (w / 2)], 0, 0, 0, 0, 0));         // Above/below the region.
	REQUIRE(s_px_near(px[(h / 2) * w + 10], 0, 0, 0, 0, 0));         // Left of the region.

	cf_free(px);
	cf_destroy_canvas(canvas);
	return true;
}

// Screen-space 3d strokes (cf_draw3d_push_stroke_pixels) measure in the same logical units
// as the 2d layer, so an 8-unit stroke covers 8 * pixel_scale device pixels on the app
// canvas -- the same on-screen width at any display density. The ortho projection maps one
// world unit to one logical unit exactly, making the rendered width directly measurable.
TEST_CASE(test_hidpi_3d_pixel_strokes_are_logical)
{
	if (!test_make_app(LOGICAL_W, LOGICAL_H)) return true; // Headless CI: no display/GPU.
	HidpiGuard guard;

	cf_app_force_pixel_scale(2.0f);
	// Latch the default projection into mvp (see test_hidpi_default_projection_is_points).
	cf_app_update(NULL);
	cf_app_draw_onto_screen(false);
	cf_app_update(NULL);

	int w = cf_app_get_canvas_width();
	int h = cf_app_get_canvas_height();
	REQUIRE(w == LOGICAL_W * 2 && h == LOGICAL_H * 2);
	CF_Pixel* px = (CF_Pixel*)cf_alloc(w * h * (int)sizeof(CF_Pixel));

	cf_draw3d_push_projection(cf_ortho(-LOGICAL_W / 2.0f, LOGICAL_W / 2.0f, -LOGICAL_H / 2.0f, LOGICAL_H / 2.0f, 0.1f, 10.0f));
	cf_draw3d_push_view(cf_look_at(cf_v3(0, 0, 1), cf_v3(0, 0, 0), cf_v3(0, 1, 0)));
	cf_draw3d_push_stroke_pixels(true);
	cf_draw3d_push_color(cf_make_color_rgb_f(1.0f, 0, 0));
	cf_draw3d_line(cf_v3(0, -50, 0), cf_v3(0, 50, 0), 8.0f); // 8 logical units thick.
	cf_draw3d_pop_color();
	cf_draw3d_pop_stroke_pixels();
	cf_draw3d_pop_view();
	cf_draw3d_pop_projection();
	cf_app_draw_onto_screen(true);

	bool ok = s_readback_canvas(cf_app_get_canvas(), w, h, px);
	REQUIRE(ok);
	// Count solidly-red pixels across the vertical line at the center row. 8 logical units
	// at pixel_scale 2 is a 16-device-pixel core; the AA fringe adds a couple more. The
	// device-pixel interpretation this guards against reads ~8 instead.
	int count = 0;
	for (int x = 0; x < w; ++x) {
		if (px[(h / 2) * w + x].colors.r > 100) count++;
	}
	if (!(count >= 13 && count <= 24)) printf("stroke width %d device px, expected ~16 in [13, 24]\n", count);
	REQUIRE(count >= 13 && count <= 24);

	cf_free(px);
	return true;
}

// A canvas recreation landing mid-recording must not corrupt the retained draw list:
// recording runs in identity space (cf_draw_list_begin) and replay composes the live
// projection on top, so a projection/mvp refresh stomped into the recording bakes the
// ortho in twice. The refresh must defer to the frame boundary instead -- which also
// makes the post-recording frames render at the NEW size even though cf_draw_list_end's
// pop restored the pre-resize projection.
TEST_CASE(test_hidpi_draw_list_recorded_across_resize)
{
	if (!test_make_app(LOGICAL_W, LOGICAL_H)) return true; // Headless CI: no display/GPU.
	HidpiGuard guard;

	cf_app_force_pixel_scale(2.0f);
	cf_app_update(NULL);
	cf_app_draw_onto_screen(false);
	cf_app_update(NULL);

	CF_DrawList list = cf_make_draw_list();
	cf_draw_list_begin(list);
	cf_app_set_size(400, 300); // Recreates the default canvas MID-recording.
	cf_draw_push_color(cf_make_color_rgb_f(1.0f, 0, 0));
	cf_draw_quad_fill(cf_make_aabb(cf_v2(-40, -40), cf_v2(40, 40)), 0); // Recorded after the resize.
	cf_draw_pop_color();
	cf_draw_list_end();
	cf_app_draw_onto_screen(false); // Frame boundary: the deferred projection refresh applies here.

	int w = cf_app_get_canvas_width();
	int h = cf_app_get_canvas_height();
	REQUIRE(w == 800 && h == 600);
	CF_Pixel* px = (CF_Pixel*)cf_alloc(w * h * (int)sizeof(CF_Pixel));

	cf_app_update(NULL);
	cf_draw_list(list);
	cf_app_draw_onto_screen(true);

	bool ok = s_readback_canvas(cf_app_get_canvas(), w, h, px);
	REQUIRE(ok);
	// The 80x80-logical quad replays centered at 160x160 device pixels on the 800x600 canvas.
	REQUIRE(s_px_near(px[(h / 2) * w + (w / 2)], 255, 0, 0, 255, 3));      // Center.
	REQUIRE(s_px_near(px[(h / 2) * w + (w / 2 + 70)], 255, 0, 0, 255, 3)); // Inside the quad.
	REQUIRE(s_px_near(px[(h / 2) * w + (w / 2 + 100)], 0, 0, 0, 0, 0));    // Outside the quad.

	cf_free(px);
	cf_destroy_draw_list(list);
	return true;
}

TEST_SUITE(test_hidpi)
{
	RUN_TEST_CASE(test_hidpi_scale_rect_rounds_edges);
	RUN_TEST_CASE(test_hidpi_forced_scale_resizes_canvas);
	RUN_TEST_CASE(test_hidpi_force_respects_no_high_dpi);
	RUN_TEST_CASE(test_hidpi_force_is_safe_without_gfx);
	RUN_TEST_CASE(test_hidpi_default_projection_is_points);
	RUN_TEST_CASE(test_hidpi_full_extent_covers_app_canvas);
	RUN_TEST_CASE(test_hidpi_first_frame_after_resize);
	RUN_TEST_CASE(test_hidpi_one_shot_canvas_keeps_points_projection);
	RUN_TEST_CASE(test_hidpi_scissor_is_logical_on_app_canvas);
	RUN_TEST_CASE(test_hidpi_viewport_is_logical_on_app_canvas);
	RUN_TEST_CASE(test_hidpi_scissor_stays_raw_on_user_canvas);
	RUN_TEST_CASE(test_hidpi_3d_pixel_strokes_are_logical);
	RUN_TEST_CASE(test_hidpi_draw_list_recorded_across_resize);
}
