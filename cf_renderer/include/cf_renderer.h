/*
	CF Renderer - A full-featured 2D renderer based on SDL3 GPU API
	Extracted from Cute Framework
	Copyright (C) 2024 Randy Gaul https://randygaul.github.io/
	
	This software is dual-licensed with zlib or Unlicense, check LICENSE.txt for more info
*/

#ifndef CF_RENDERER_H
#define CF_RENDERER_H

#include <SDL3/SDL.h>
#include <SDL3/SDL_gpu.h>
#include <stdint.h>
#include <stdbool.h>
#include <math.h>

#ifdef __cplusplus
extern "C" {
#endif

//--------------------------------------------------------------------------------------------------
// Core types - Using SDL3 types directly
//--------------------------------------------------------------------------------------------------

typedef struct CFR_Renderer CFR_Renderer;

// Math types - Simple wrappers for convenience
typedef struct CFR_V2 {
	float x, y;
} CFR_V2;

typedef struct CFR_Aabb {
	CFR_V2 min, max;
} CFR_Aabb;

typedef struct CFR_Circle {
	CFR_V2 p;
	float r;
} CFR_Circle;

typedef struct CFR_M3x2 {
	CFR_V2 x;
	CFR_V2 y;
	CFR_V2 p;
} CFR_M3x2;

typedef struct CFR_Transform {
	CFR_V2 p;
	struct { float c, s; } r;  // cos, sin of rotation
} CFR_Transform;

// Pixel type
typedef struct CFR_Pixel {
	uint8_t r, g, b, a;
} CFR_Pixel;

// Opaque handles for resources
typedef struct { uint64_t id; } CFR_Texture;
typedef struct { uint64_t id; } CFR_Shader;
typedef struct { uint64_t id; } CFR_Canvas;
typedef struct { uint64_t id; } CFR_Mesh;
typedef struct { uint64_t id; } CFR_Material;

//--------------------------------------------------------------------------------------------------
// Renderer lifecycle
//--------------------------------------------------------------------------------------------------

/**
 * Create a new renderer instance
 * @param device SDL GPU device to use for rendering
 * @param window SDL window to render to
 * @param width Initial viewport width
 * @param height Initial viewport height
 * @return Renderer instance or NULL on failure
 */
CFR_Renderer* cfr_create_renderer(SDL_GPUDevice* device, SDL_Window* window, int width, int height);

/**
 * Destroy a renderer instance and free all resources
 * @param renderer Renderer to destroy
 */
void cfr_destroy_renderer(CFR_Renderer* renderer);

/**
 * Begin a new frame
 * @param renderer Renderer instance
 */
void cfr_begin_frame(CFR_Renderer* renderer);

/**
 * End the current frame and present to screen
 * @param renderer Renderer instance
 */
void cfr_end_frame(CFR_Renderer* renderer);

/**
 * Set the viewport size (call when window is resized)
 * @param renderer Renderer instance
 * @param width New width
 * @param height New height
 */
void cfr_set_viewport_size(CFR_Renderer* renderer, int width, int height);

//--------------------------------------------------------------------------------------------------
// Low-level resource management
//--------------------------------------------------------------------------------------------------

// Texture creation using SDL3 types directly
CFR_Texture cfr_make_texture(CFR_Renderer* renderer, int width, int height, SDL_GPUTextureFormat format);
void cfr_texture_update(CFR_Renderer* renderer, CFR_Texture texture, void* pixels, int size);
void cfr_destroy_texture(CFR_Renderer* renderer, CFR_Texture texture);

// Canvas (render target) management
CFR_Canvas cfr_make_canvas(CFR_Renderer* renderer, int width, int height);
void cfr_destroy_canvas(CFR_Renderer* renderer, CFR_Canvas canvas);
CFR_Texture cfr_canvas_get_texture(CFR_Renderer* renderer, CFR_Canvas canvas);

// Render to canvas or screen (canvas ID 0 = screen)
void cfr_render_to(CFR_Renderer* renderer, CFR_Canvas canvas, bool clear);

//--------------------------------------------------------------------------------------------------
// Drawing state management
//--------------------------------------------------------------------------------------------------

// Color for drawing
void cfr_push_color(CFR_Renderer* renderer, SDL_FColor color);
SDL_FColor cfr_pop_color(CFR_Renderer* renderer);
SDL_FColor cfr_peek_color(CFR_Renderer* renderer);

// Layer ordering
void cfr_push_layer(CFR_Renderer* renderer, int layer);
int cfr_pop_layer(CFR_Renderer* renderer);
int cfr_peek_layer(CFR_Renderer* renderer);

// Antialiasing
void cfr_push_antialias(CFR_Renderer* renderer, bool enabled);
bool cfr_pop_antialias(CFR_Renderer* renderer);
bool cfr_peek_antialias(CFR_Renderer* renderer);

void cfr_push_antialias_scale(CFR_Renderer* renderer, float scale);
float cfr_pop_antialias_scale(CFR_Renderer* renderer);
float cfr_peek_antialias_scale(CFR_Renderer* renderer);

// Viewport and scissor
void cfr_push_viewport(CFR_Renderer* renderer, SDL_FRect viewport);
SDL_FRect cfr_pop_viewport(CFR_Renderer* renderer);
SDL_FRect cfr_peek_viewport(CFR_Renderer* renderer);

void cfr_push_scissor(CFR_Renderer* renderer, SDL_FRect scissor);
SDL_FRect cfr_pop_scissor(CFR_Renderer* renderer);
SDL_FRect cfr_peek_scissor(CFR_Renderer* renderer);

// Blend state - uses SDL3 types directly
void cfr_push_blend_state(CFR_Renderer* renderer, SDL_GPUColorTargetBlendState blend);
SDL_GPUColorTargetBlendState cfr_pop_blend_state(CFR_Renderer* renderer);
SDL_GPUColorTargetBlendState cfr_peek_blend_state(CFR_Renderer* renderer);

//--------------------------------------------------------------------------------------------------
// Transformation stack
//--------------------------------------------------------------------------------------------------

void cfr_push_transform(CFR_Renderer* renderer, CFR_M3x2 transform);
CFR_M3x2 cfr_pop_transform(CFR_Renderer* renderer);
CFR_M3x2 cfr_peek_transform(CFR_Renderer* renderer);

void cfr_translate(CFR_Renderer* renderer, float x, float y);
void cfr_rotate(CFR_Renderer* renderer, float radians);
void cfr_scale(CFR_Renderer* renderer, float sx, float sy);

// Transform a point by current matrix
CFR_V2 cfr_transform_point(CFR_Renderer* renderer, CFR_V2 point);

//--------------------------------------------------------------------------------------------------
// High-level drawing functions - Sprites
//--------------------------------------------------------------------------------------------------

/**
 * Simple sprite structure
 */
typedef struct CFR_Sprite {
	CFR_Texture texture;
	CFR_Transform transform;
	CFR_V2 scale;
	int w, h;
	float opacity;
	CFR_V2 offset;
} CFR_Sprite;

void cfr_draw_sprite(CFR_Renderer* renderer, const CFR_Sprite* sprite);

//--------------------------------------------------------------------------------------------------
// High-level drawing functions - Shapes
//--------------------------------------------------------------------------------------------------

// Lines
void cfr_draw_line(CFR_Renderer* renderer, CFR_V2 p0, CFR_V2 p1, float thickness);

// Circles
void cfr_draw_circle(CFR_Renderer* renderer, CFR_Circle circle, float thickness);
void cfr_draw_circle_fill(CFR_Renderer* renderer, CFR_Circle circle);

// Quads/Boxes
void cfr_draw_quad(CFR_Renderer* renderer, CFR_Aabb bb, float thickness, float chubbiness);
void cfr_draw_quad_fill(CFR_Renderer* renderer, CFR_Aabb bb, float chubbiness);
void cfr_draw_quad2(CFR_Renderer* renderer, CFR_V2 p0, CFR_V2 p1, CFR_V2 p2, CFR_V2 p3, float thickness, float chubbiness);
void cfr_draw_quad_fill2(CFR_Renderer* renderer, CFR_V2 p0, CFR_V2 p1, CFR_V2 p2, CFR_V2 p3, float chubbiness);

// Rounded boxes
void cfr_draw_box_rounded(CFR_Renderer* renderer, CFR_Aabb bb, float thickness, float radius);
void cfr_draw_box_rounded_fill(CFR_Renderer* renderer, CFR_Aabb bb, float radius);

// Capsules
void cfr_draw_capsule(CFR_Renderer* renderer, CFR_V2 p0, CFR_V2 p1, float r, float thickness);
void cfr_draw_capsule_fill(CFR_Renderer* renderer, CFR_V2 p0, CFR_V2 p1, float r);

// Triangles
void cfr_draw_tri(CFR_Renderer* renderer, CFR_V2 p0, CFR_V2 p1, CFR_V2 p2, float thickness, float chubbiness);
void cfr_draw_tri_fill(CFR_Renderer* renderer, CFR_V2 p0, CFR_V2 p1, CFR_V2 p2, float chubbiness);

// Polygons
void cfr_draw_polygon(CFR_Renderer* renderer, CFR_V2* points, int count, float thickness);
void cfr_draw_polygon_fill(CFR_Renderer* renderer, CFR_V2* points, int count);

// Bezier curves
void cfr_draw_bezier_line(CFR_Renderer* renderer, CFR_V2 a, CFR_V2 c0, CFR_V2 b, float thickness);
void cfr_draw_bezier_line2(CFR_Renderer* renderer, CFR_V2 a, CFR_V2 c0, CFR_V2 c1, CFR_V2 b, float thickness);

//--------------------------------------------------------------------------------------------------
// Utility functions
//--------------------------------------------------------------------------------------------------

// Math helpers
CFR_V2 cfr_v2(float x, float y);
CFR_V2 cfr_add(CFR_V2 a, CFR_V2 b);
CFR_V2 cfr_sub(CFR_V2 a, CFR_V2 b);
CFR_V2 cfr_mul(CFR_V2 v, float s);
float cfr_dot(CFR_V2 a, CFR_V2 b);
float cfr_len(CFR_V2 v);
CFR_V2 cfr_norm(CFR_V2 v);

CFR_Aabb cfr_make_aabb(CFR_V2 min, CFR_V2 max);
CFR_Aabb cfr_make_aabb_center_half_extents(CFR_V2 center, CFR_V2 half_extents);
CFR_Circle cfr_make_circle(CFR_V2 p, float r);

CFR_M3x2 cfr_make_identity(void);
CFR_M3x2 cfr_make_translation(float x, float y);
CFR_M3x2 cfr_make_scale(float sx, float sy);
CFR_M3x2 cfr_make_rotation(float radians);
CFR_M3x2 cfr_mul_m3x2(CFR_M3x2 a, CFR_M3x2 b);
CFR_V2 cfr_mul_m3x2_v2(CFR_M3x2 m, CFR_V2 v);

// Color helpers
SDL_FColor cfr_make_color(uint8_t r, uint8_t g, uint8_t b, uint8_t a);
SDL_FColor cfr_make_color_f(float r, float g, float b, float a);
SDL_FColor cfr_make_color_hex(uint32_t hex);

// Sprite helpers
CFR_Sprite cfr_make_sprite(CFR_Texture texture, int width, int height);

// Default blend states
SDL_GPUColorTargetBlendState cfr_blend_state_default(void);
SDL_GPUColorTargetBlendState cfr_blend_state_alpha(void);
SDL_GPUColorTargetBlendState cfr_blend_state_additive(void);
SDL_GPUColorTargetBlendState cfr_blend_state_multiplicative(void);

#ifdef __cplusplus
}
#endif

#endif // CF_RENDERER_H
