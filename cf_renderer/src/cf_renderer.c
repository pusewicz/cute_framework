/*
	CF Renderer - A full-featured 2D renderer based on SDL3 GPU API
	Extracted from Cute Framework
	Copyright (C) 2024 Randy Gaul https://randygaul.github.io/
	
	This software is dual-licensed with zlib or Unlicense, check LICENSE.txt for more info
*/

#include "../include/cf_renderer.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

//--------------------------------------------------------------------------------------------------
// Spritebatch integration
//--------------------------------------------------------------------------------------------------

#define SPRITEBATCH_U64 uint64_t
#define SPRITEBATCH_ASSERT assert
#define SPRITEBATCH_IMPLEMENTATION

// Sprite geometry for batching
typedef struct BatchGeometry {
	int type;
	SDL_FColor color;
	CFR_V2 shape[8];
	float alpha;
	float radius;
	float stroke;
	float aa;
	bool fill;
} BatchGeometry;

#define SPRITEBATCH_SPRITE_GEOMETRY BatchGeometry

#include "../../libraries/cute/cute_spritebatch.h"

//--------------------------------------------------------------------------------------------------
// Internal structures
//--------------------------------------------------------------------------------------------------

#define CFR_MAX_TEXTURES 1024
#define CFR_MAX_CANVASES 64
#define CFR_MAX_LAYERS 128

// Geometry types for batch rendering
enum {
	GEOM_TYPE_SPRITE = 0,
	GEOM_TYPE_QUAD = 1,
	GEOM_TYPE_CIRCLE = 2,
	GEOM_TYPE_LINE = 3,
	GEOM_TYPE_TRIANGLE = 4,
	GEOM_TYPE_POLYGON = 5
};

typedef struct CFR_Vertex {
	CFR_V2 pos;
	CFR_V2 posH;  // Homogenous position (after transform)
	CFR_V2 uv;
	SDL_FColor color;
	float radius;
	float stroke;
	float aa;
	CFR_V2 shape[8];
	int type;
	uint8_t alpha;
	uint8_t fill;
} CFR_Vertex;

typedef struct CFR_TextureInternal {
	SDL_GPUTexture* texture;
	SDL_GPUSampler* sampler;
	SDL_GPUTransferBuffer* transfer_buffer;
	int width;
	int height;
	SDL_GPUTextureFormat format;
	bool in_use;
} CFR_TextureInternal;

typedef struct CFR_CanvasInternal {
	SDL_GPUTexture* texture;
	SDL_GPUSampler* sampler;
	int width;
	int height;
	bool in_use;
} CFR_CanvasInternal;

typedef struct CFR_DrawCommand {
	int layer;
	int id;
	SDL_FRect scissor;
	SDL_FRect viewport;
	SDL_GPUColorTargetBlendState blend;
	void* items;  // Array of sprite batch items
	int item_count;
	bool processed;
} CFR_DrawCommand;

struct CFR_Renderer {
	SDL_GPUDevice* device;
	SDL_Window* window;
	SDL_GPUCommandBuffer* cmd_buffer;
	SDL_GPURenderPass* render_pass;
	SDL_GPUTexture* swapchain_texture;
	
	int width, height;
	
	// Resource pools
	CFR_TextureInternal textures[CFR_MAX_TEXTURES];
	CFR_CanvasInternal canvases[CFR_MAX_CANVASES];
	
	// Sprite batching
	spritebatch_t sprite_batch;
	
	// Rendering state stacks
	SDL_FColor* color_stack;
	int color_stack_count;
	int color_stack_capacity;
	
	int* layer_stack;
	int layer_stack_count;
	int layer_stack_capacity;
	
	bool* antialias_stack;
	int antialias_stack_count;
	int antialias_stack_capacity;
	
	float* antialias_scale_stack;
	int antialias_scale_stack_count;
	int antialias_scale_stack_capacity;
	
	SDL_FRect* viewport_stack;
	int viewport_stack_count;
	int viewport_stack_capacity;
	
	SDL_FRect* scissor_stack;
	int scissor_stack_count;
	int scissor_stack_capacity;
	
	SDL_GPUColorTargetBlendState* blend_stack;
	int blend_stack_count;
	int blend_stack_capacity;
	
	CFR_M3x2* transform_stack;
	int transform_stack_count;
	int transform_stack_capacity;
	
	// Current state
	CFR_M3x2 projection;
	CFR_M3x2 mvp;
	float aaf;  // Antialiasing factor
	
	// Rendering resources
	SDL_GPUShader* default_vs;
	SDL_GPUShader* default_fs;
	SDL_GPUBuffer* vertex_buffer;
	SDL_GPUTransferBuffer* vertex_transfer;
	
	// Draw commands
	CFR_DrawCommand* commands;
	int command_count;
	int command_capacity;
	int command_id_counter;
	
	bool frame_begun;
};

//--------------------------------------------------------------------------------------------------
// Stack management helpers
//--------------------------------------------------------------------------------------------------

#define STACK_PUSH(stack, type, item) do { \
	if (renderer->stack##_stack_count >= renderer->stack##_stack_capacity) { \
		int new_cap = renderer->stack##_stack_capacity == 0 ? 16 : renderer->stack##_stack_capacity * 2; \
		renderer->stack##_stack = (type*)realloc(renderer->stack##_stack, new_cap * sizeof(type)); \
		renderer->stack##_stack_capacity = new_cap; \
	} \
	renderer->stack##_stack[renderer->stack##_stack_count++] = item; \
} while(0)

#define STACK_POP(stack, type, default_val) \
	(renderer->stack##_stack_count > 1 ? renderer->stack##_stack[--renderer->stack##_stack_count] : \
	 (renderer->stack##_stack_count > 0 ? renderer->stack##_stack[0] : default_val))

#define STACK_PEEK(stack, type, default_val) \
	(renderer->stack##_stack_count > 0 ? renderer->stack##_stack[renderer->stack##_stack_count - 1] : default_val)

//--------------------------------------------------------------------------------------------------
// Spritebatch callbacks
//--------------------------------------------------------------------------------------------------

static SPRITEBATCH_U64 cfr_generate_texture_handle(void* pixels, int w, int h, void* udata)
{
	CFR_Renderer* renderer = (CFR_Renderer*)udata;
	
	CFR_Texture tex = cfr_make_texture(renderer, w, h, SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM);
	if (tex.id) {
		cfr_texture_update(renderer, tex, pixels, w * h * 4);
	}
	return tex.id;
}

static void cfr_destroy_texture_handle(SPRITEBATCH_U64 texture_id, void* udata)
{
	CFR_Renderer* renderer = (CFR_Renderer*)udata;
	CFR_Texture tex = { texture_id };
	cfr_destroy_texture(renderer, tex);
}

static void cfr_get_pixels_callback(SPRITEBATCH_U64 image_id, void* buffer, int bytes_to_fill, void* udata)
{
	// For now, we don't support getting pixels back from GPU
	memset(buffer, 0, bytes_to_fill);
}

static void cfr_batch_report(spritebatch_sprite_t* sprites, int count, int texture_w, int texture_h, void* udata)
{
	CFR_Renderer* renderer = (CFR_Renderer*)udata;
	
	// Convert sprites to vertices and submit draw call
	// This would be implemented with the actual rendering code
	// For now, this is a placeholder
}

//--------------------------------------------------------------------------------------------------
// Math helpers
//--------------------------------------------------------------------------------------------------

CFR_V2 cfr_v2(float x, float y)
{
	CFR_V2 v = { x, y };
	return v;
}

CFR_V2 cfr_add(CFR_V2 a, CFR_V2 b)
{
	return cfr_v2(a.x + b.x, a.y + b.y);
}

CFR_V2 cfr_sub(CFR_V2 a, CFR_V2 b)
{
	return cfr_v2(a.x - b.x, a.y - b.y);
}

CFR_V2 cfr_mul(CFR_V2 v, float s)
{
	return cfr_v2(v.x * s, v.y * s);
}

float cfr_dot(CFR_V2 a, CFR_V2 b)
{
	return a.x * b.x + a.y * b.y;
}

float cfr_len(CFR_V2 v)
{
	return sqrtf(v.x * v.x + v.y * v.y);
}

CFR_V2 cfr_norm(CFR_V2 v)
{
	float len = cfr_len(v);
	if (len > 0.0f) {
		return cfr_mul(v, 1.0f / len);
	}
	return v;
}

CFR_Aabb cfr_make_aabb(CFR_V2 min, CFR_V2 max)
{
	CFR_Aabb bb = { min, max };
	return bb;
}

CFR_Aabb cfr_make_aabb_center_half_extents(CFR_V2 center, CFR_V2 half_extents)
{
	return cfr_make_aabb(
		cfr_sub(center, half_extents),
		cfr_add(center, half_extents)
	);
}

CFR_Circle cfr_make_circle(CFR_V2 p, float r)
{
	CFR_Circle c = { p, r };
	return c;
}

CFR_M3x2 cfr_make_identity(void)
{
	CFR_M3x2 m = {
		{ 1, 0 },
		{ 0, 1 },
		{ 0, 0 }
	};
	return m;
}

CFR_M3x2 cfr_make_translation(float x, float y)
{
	CFR_M3x2 m = {
		{ 1, 0 },
		{ 0, 1 },
		{ x, y }
	};
	return m;
}

CFR_M3x2 cfr_make_scale(float sx, float sy)
{
	CFR_M3x2 m = {
		{ sx, 0 },
		{ 0, sy },
		{ 0, 0 }
	};
	return m;
}

CFR_M3x2 cfr_make_rotation(float radians)
{
	float c = cosf(radians);
	float s = sinf(radians);
	CFR_M3x2 m = {
		{ c, s },
		{ -s, c },
		{ 0, 0 }
	};
	return m;
}

CFR_M3x2 cfr_mul_m3x2(CFR_M3x2 a, CFR_M3x2 b)
{
	CFR_M3x2 result;
	result.x.x = a.x.x * b.x.x + a.y.x * b.x.y;
	result.x.y = a.x.y * b.x.x + a.y.y * b.x.y;
	result.y.x = a.x.x * b.y.x + a.y.x * b.y.y;
	result.y.y = a.x.y * b.y.x + a.y.y * b.y.y;
	result.p.x = a.x.x * b.p.x + a.y.x * b.p.y + a.p.x;
	result.p.y = a.x.y * b.p.x + a.y.y * b.p.y + a.p.y;
	return result;
}

CFR_V2 cfr_mul_m3x2_v2(CFR_M3x2 m, CFR_V2 v)
{
	CFR_V2 result;
	result.x = m.x.x * v.x + m.y.x * v.y + m.p.x;
	result.y = m.x.y * v.x + m.y.y * v.y + m.p.y;
	return result;
}

SDL_FColor cfr_make_color(uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
	SDL_FColor color = {
		r / 255.0f,
		g / 255.0f,
		b / 255.0f,
		a / 255.0f
	};
	return color;
}

SDL_FColor cfr_make_color_f(float r, float g, float b, float a)
{
	SDL_FColor color = { r, g, b, a };
	return color;
}

SDL_FColor cfr_make_color_hex(uint32_t hex)
{
	return cfr_make_color(
		(hex >> 24) & 0xFF,
		(hex >> 16) & 0xFF,
		(hex >> 8) & 0xFF,
		hex & 0xFF
	);
}

CFR_Sprite cfr_make_sprite(CFR_Texture texture, int width, int height)
{
	CFR_Sprite sprite = {0};
	sprite.texture = texture;
	sprite.w = width;
	sprite.h = height;
	sprite.transform.p = cfr_v2(0, 0);
	sprite.transform.r.c = 1;
	sprite.transform.r.s = 0;
	sprite.scale = cfr_v2(1, 1);
	sprite.opacity = 1.0f;
	sprite.offset = cfr_v2(0, 0);
	return sprite;
}

SDL_GPUColorTargetBlendState cfr_blend_state_default(void)
{
	SDL_GPUColorTargetBlendState blend = {
		.src_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE,
		.dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ZERO,
		.color_blend_op = SDL_GPU_BLENDOP_ADD,
		.src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE,
		.dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ZERO,
		.alpha_blend_op = SDL_GPU_BLENDOP_ADD,
		.color_write_mask = 0xF,
		.enable_blend = false,
		.enable_color_write_mask = false
	};
	return blend;
}

SDL_GPUColorTargetBlendState cfr_blend_state_alpha(void)
{
	SDL_GPUColorTargetBlendState blend = {
		.src_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE,
		.dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
		.color_blend_op = SDL_GPU_BLENDOP_ADD,
		.src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE,
		.dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
		.alpha_blend_op = SDL_GPU_BLENDOP_ADD,
		.color_write_mask = 0xF,
		.enable_blend = true,
		.enable_color_write_mask = false
	};
	return blend;
}

SDL_GPUColorTargetBlendState cfr_blend_state_additive(void)
{
	SDL_GPUColorTargetBlendState blend = {
		.src_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE,
		.dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ONE,
		.color_blend_op = SDL_GPU_BLENDOP_ADD,
		.src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE,
		.dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ONE,
		.alpha_blend_op = SDL_GPU_BLENDOP_ADD,
		.color_write_mask = 0xF,
		.enable_blend = true,
		.enable_color_write_mask = false
	};
	return blend;
}

SDL_GPUColorTargetBlendState cfr_blend_state_multiplicative(void)
{
	SDL_GPUColorTargetBlendState blend = {
		.src_color_blendfactor = SDL_GPU_BLENDFACTOR_DST_COLOR,
		.dst_color_blendfactor = SDL_GPU_BLENDFACTOR_ZERO,
		.color_blend_op = SDL_GPU_BLENDOP_ADD,
		.src_alpha_blendfactor = SDL_GPU_BLENDFACTOR_DST_ALPHA,
		.dst_alpha_blendfactor = SDL_GPU_BLENDFACTOR_ZERO,
		.alpha_blend_op = SDL_GPU_BLENDOP_ADD,
		.color_write_mask = 0xF,
		.enable_blend = true,
		.enable_color_write_mask = false
	};
	return blend;
}

//--------------------------------------------------------------------------------------------------
// Renderer lifecycle
//--------------------------------------------------------------------------------------------------

CFR_Renderer* cfr_create_renderer(SDL_GPUDevice* device, SDL_Window* window, int width, int height)
{
	if (!device || !window) {
		return NULL;
	}
	
	CFR_Renderer* renderer = (CFR_Renderer*)calloc(1, sizeof(CFR_Renderer));
	if (!renderer) {
		return NULL;
	}
	
	renderer->device = device;
	renderer->window = window;
	renderer->width = width;
	renderer->height = height;
	renderer->frame_begun = false;
	
	// Initialize sprite batch
	spritebatch_config_t config;
	spritebatch_set_default_config(&config);
	config.atlas_use_border_pixels = 1;
	config.batch_callback = cfr_batch_report;
	config.get_pixels_callback = cfr_get_pixels_callback;
	config.generate_texture_callback = cfr_generate_texture_handle;
	config.delete_texture_callback = cfr_destroy_texture_handle;
	config.udata = renderer;
	spritebatch_init(&renderer->sprite_batch, &config, NULL);
	
	// Initialize stacks with defaults
	SDL_FColor white = { 1, 1, 1, 1 };
	STACK_PUSH(color, SDL_FColor, white);
	
	int layer_default = 0;
	STACK_PUSH(layer, int, layer_default);
	
	bool aa_default = true;
	STACK_PUSH(antialias, bool, aa_default);
	
	float aa_scale_default = 1.5f;
	STACK_PUSH(antialias_scale, float, aa_scale_default);
	
	SDL_FRect viewport_default = { 0, 0, (float)width, (float)height };
	STACK_PUSH(viewport, SDL_FRect, viewport_default);
	
	SDL_FRect scissor_default = { 0, 0, -1, -1 };
	STACK_PUSH(scissor, SDL_FRect, scissor_default);
	
	SDL_GPUColorTargetBlendState blend_default = cfr_blend_state_alpha();
	STACK_PUSH(blend, SDL_GPUColorTargetBlendState, blend_default);
	
	CFR_M3x2 transform_default = cfr_make_identity();
	STACK_PUSH(transform, CFR_M3x2, transform_default);
	
	// Setup projection (orthographic, origin at top-left)
	renderer->projection = cfr_make_identity();
	renderer->projection.x.x = 2.0f / width;
	renderer->projection.y.y = -2.0f / height;
	renderer->projection.p.x = -1.0f;
	renderer->projection.p.y = 1.0f;
	renderer->mvp = renderer->projection;
	
	return renderer;
}

void cfr_destroy_renderer(CFR_Renderer* renderer)
{
	if (!renderer) {
		return;
	}
	
	// Clean up textures
	for (int i = 0; i < CFR_MAX_TEXTURES; i++) {
		if (renderer->textures[i].in_use) {
			if (renderer->textures[i].texture) {
				SDL_ReleaseGPUTexture(renderer->device, renderer->textures[i].texture);
			}
			if (renderer->textures[i].sampler) {
				SDL_ReleaseGPUSampler(renderer->device, renderer->textures[i].sampler);
			}
			if (renderer->textures[i].transfer_buffer) {
				SDL_ReleaseGPUTransferBuffer(renderer->device, renderer->textures[i].transfer_buffer);
			}
		}
	}
	
	// Clean up canvases
	for (int i = 0; i < CFR_MAX_CANVASES; i++) {
		if (renderer->canvases[i].in_use) {
			if (renderer->canvases[i].texture) {
				SDL_ReleaseGPUTexture(renderer->device, renderer->canvases[i].texture);
			}
			if (renderer->canvases[i].sampler) {
				SDL_ReleaseGPUSampler(renderer->device, renderer->canvases[i].sampler);
			}
		}
	}
	
	// Clean up sprite batch
	spritebatch_term(&renderer->sprite_batch);
	
	// Clean up stacks
	free(renderer->color_stack);
	free(renderer->layer_stack);
	free(renderer->antialias_stack);
	free(renderer->antialias_scale_stack);
	free(renderer->viewport_stack);
	free(renderer->scissor_stack);
	free(renderer->blend_stack);
	free(renderer->transform_stack);
	free(renderer->commands);
	
	free(renderer);
}

void cfr_begin_frame(CFR_Renderer* renderer)
{
	if (!renderer || renderer->frame_begun) {
		return;
	}
	
	renderer->cmd_buffer = SDL_AcquireGPUCommandBuffer(renderer->device);
	if (!renderer->cmd_buffer) {
		return;
	}
	
	renderer->frame_begun = true;
}

void cfr_end_frame(CFR_Renderer* renderer)
{
	if (!renderer || !renderer->frame_begun) {
		return;
	}
	
	// Flush any pending sprite batch commands
	spritebatch_flush(&renderer->sprite_batch);
	
	if (renderer->render_pass) {
		SDL_EndGPURenderPass(renderer->render_pass);
		renderer->render_pass = NULL;
	}
	
	if (renderer->cmd_buffer) {
		SDL_SubmitGPUCommandBuffer(renderer->cmd_buffer);
		renderer->cmd_buffer = NULL;
	}
	
	renderer->frame_begun = false;
}

void cfr_set_viewport_size(CFR_Renderer* renderer, int width, int height)
{
	if (!renderer) return;
	
	renderer->width = width;
	renderer->height = height;
	
	// Update projection matrix
	renderer->projection = cfr_make_identity();
	renderer->projection.x.x = 2.0f / width;
	renderer->projection.y.y = -2.0f / height;
	renderer->projection.p.x = -1.0f;
	renderer->projection.p.y = 1.0f;
	renderer->mvp = cfr_mul_m3x2(renderer->projection, STACK_PEEK(transform, CFR_M3x2, cfr_make_identity()));
}

//--------------------------------------------------------------------------------------------------
// Low-level resource management
//--------------------------------------------------------------------------------------------------

CFR_Texture cfr_make_texture(CFR_Renderer* renderer, int width, int height, SDL_GPUTextureFormat format)
{
	CFR_Texture result = { 0 };
	if (!renderer) return result;
	
	// Find free slot
	int slot = -1;
	for (int i = 0; i < CFR_MAX_TEXTURES; i++) {
		if (!renderer->textures[i].in_use) {
			slot = i;
			break;
		}
	}
	
	if (slot == -1) return result;
	
	CFR_TextureInternal* tex = &renderer->textures[slot];
	
	SDL_GPUTextureCreateInfo create_info = {
		.type = SDL_GPU_TEXTURETYPE_2D,
		.format = format,
		.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER,
		.width = width,
		.height = height,
		.layer_count_or_depth = 1,
		.num_levels = 1,
		.sample_count = SDL_GPU_SAMPLECOUNT_1,
		.props = 0
	};
	
	tex->texture = SDL_CreateGPUTexture(renderer->device, &create_info);
	if (!tex->texture) return result;
	
	SDL_GPUSamplerCreateInfo sampler_info = {
		.min_filter = SDL_GPU_FILTER_LINEAR,
		.mag_filter = SDL_GPU_FILTER_LINEAR,
		.mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_NEAREST,
		.address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
		.address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
		.address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
		.props = 0
	};
	
	tex->sampler = SDL_CreateGPUSampler(renderer->device, &sampler_info);
	if (!tex->sampler) {
		SDL_ReleaseGPUTexture(renderer->device, tex->texture);
		tex->texture = NULL;
		return result;
	}
	
	tex->width = width;
	tex->height = height;
	tex->format = format;
	tex->in_use = true;
	
	result.id = (uint64_t)(slot + 1);
	return result;
}

void cfr_texture_update(CFR_Renderer* renderer, CFR_Texture texture, void* pixels, int size)
{
	if (!renderer || !texture.id || !pixels) return;
	
	int slot = (int)(texture.id - 1);
	if (slot < 0 || slot >= CFR_MAX_TEXTURES || !renderer->textures[slot].in_use) return;
	
	CFR_TextureInternal* tex = &renderer->textures[slot];
	
	// Create transfer buffer if needed
	if (!tex->transfer_buffer) {
		SDL_GPUTransferBufferCreateInfo transfer_info = {
			.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
			.size = size,
			.props = 0
		};
		tex->transfer_buffer = SDL_CreateGPUTransferBuffer(renderer->device, &transfer_info);
	}
	
	if (!tex->transfer_buffer) return;
	
	// Map and copy data
	void* mapped = SDL_MapGPUTransferBuffer(renderer->device, tex->transfer_buffer, false);
	if (mapped) {
		memcpy(mapped, pixels, size);
		SDL_UnmapGPUTransferBuffer(renderer->device, tex->transfer_buffer);
	}
	
	// Upload to texture
	SDL_GPUCommandBuffer* upload_cmd = SDL_AcquireGPUCommandBuffer(renderer->device);
	if (upload_cmd) {
		SDL_GPUCopyPass* copy_pass = SDL_BeginGPUCopyPass(upload_cmd);
		if (copy_pass) {
			SDL_GPUTextureTransferInfo src = {
				.transfer_buffer = tex->transfer_buffer,
				.offset = 0,
				.pixels_per_row = tex->width,
				.rows_per_layer = tex->height
			};
			
			SDL_GPUTextureRegion dst = {
				.texture = tex->texture,
				.mip_level = 0,
				.layer = 0,
				.x = 0,
				.y = 0,
				.z = 0,
				.w = tex->width,
				.h = tex->height,
				.d = 1
			};
			
			SDL_UploadToGPUTexture(copy_pass, &src, &dst, false);
			SDL_EndGPUCopyPass(copy_pass);
		}
		SDL_SubmitGPUCommandBuffer(upload_cmd);
	}
}

void cfr_destroy_texture(CFR_Renderer* renderer, CFR_Texture texture)
{
	if (!renderer || !texture.id) return;
	
	int slot = (int)(texture.id - 1);
	if (slot < 0 || slot >= CFR_MAX_TEXTURES || !renderer->textures[slot].in_use) return;
	
	CFR_TextureInternal* tex = &renderer->textures[slot];
	
	if (tex->texture) {
		SDL_ReleaseGPUTexture(renderer->device, tex->texture);
		tex->texture = NULL;
	}
	
	if (tex->sampler) {
		SDL_ReleaseGPUSampler(renderer->device, tex->sampler);
		tex->sampler = NULL;
	}
	
	if (tex->transfer_buffer) {
		SDL_ReleaseGPUTransferBuffer(renderer->device, tex->transfer_buffer);
		tex->transfer_buffer = NULL;
	}
	
	tex->in_use = false;
}

CFR_Canvas cfr_make_canvas(CFR_Renderer* renderer, int width, int height)
{
	CFR_Canvas result = { 0 };
	if (!renderer) return result;
	
	// Find free slot
	int slot = -1;
	for (int i = 0; i < CFR_MAX_CANVASES; i++) {
		if (!renderer->canvases[i].in_use) {
			slot = i;
			break;
		}
	}
	
	if (slot == -1) return result;
	
	CFR_CanvasInternal* canvas = &renderer->canvases[slot];
	
	SDL_GPUTextureCreateInfo create_info = {
		.type = SDL_GPU_TEXTURETYPE_2D,
		.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM,
		.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER | SDL_GPU_TEXTUREUSAGE_COLOR_TARGET,
		.width = width,
		.height = height,
		.layer_count_or_depth = 1,
		.num_levels = 1,
		.sample_count = SDL_GPU_SAMPLECOUNT_1,
		.props = 0
	};
	
	canvas->texture = SDL_CreateGPUTexture(renderer->device, &create_info);
	if (!canvas->texture) return result;
	
	SDL_GPUSamplerCreateInfo sampler_info = {
		.min_filter = SDL_GPU_FILTER_LINEAR,
		.mag_filter = SDL_GPU_FILTER_LINEAR,
		.mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_NEAREST,
		.address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
		.address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
		.address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
		.props = 0
	};
	
	canvas->sampler = SDL_CreateGPUSampler(renderer->device, &sampler_info);
	if (!canvas->sampler) {
		SDL_ReleaseGPUTexture(renderer->device, canvas->texture);
		canvas->texture = NULL;
		return result;
	}
	
	canvas->width = width;
	canvas->height = height;
	canvas->in_use = true;
	
	result.id = (uint64_t)(slot + 1);
	return result;
}

void cfr_destroy_canvas(CFR_Renderer* renderer, CFR_Canvas canvas)
{
	if (!renderer || !canvas.id) return;
	
	int slot = (int)(canvas.id - 1);
	if (slot < 0 || slot >= CFR_MAX_CANVASES || !renderer->canvases[slot].in_use) return;
	
	CFR_CanvasInternal* c = &renderer->canvases[slot];
	
	if (c->texture) {
		SDL_ReleaseGPUTexture(renderer->device, c->texture);
		c->texture = NULL;
	}
	
	if (c->sampler) {
		SDL_ReleaseGPUSampler(renderer->device, c->sampler);
		c->sampler = NULL;
	}
	
	c->in_use = false;
}

CFR_Texture cfr_canvas_get_texture(CFR_Renderer* renderer, CFR_Canvas canvas)
{
	CFR_Texture result = { 0 };
	if (!renderer || !canvas.id) return result;
	
	// Canvas ID is the same as texture ID in this implementation
	result.id = canvas.id;
	return result;
}

void cfr_render_to(CFR_Renderer* renderer, CFR_Canvas canvas, bool clear)
{
	if (!renderer) return;
	
	// End current render pass if active
	if (renderer->render_pass) {
		SDL_EndGPURenderPass(renderer->render_pass);
		renderer->render_pass = NULL;
	}
	
	SDL_GPUTexture* target_texture;
	
	if (canvas.id == 0) {
		// Render to swapchain
		if (!renderer->cmd_buffer) return;
		
		if (!SDL_WaitAndAcquireGPUSwapchainTexture(renderer->cmd_buffer, renderer->window, &target_texture, NULL, NULL)) {
			return;
		}
		renderer->swapchain_texture = target_texture;
	} else {
		// Render to canvas
		int slot = (int)(canvas.id - 1);
		if (slot < 0 || slot >= CFR_MAX_CANVASES || !renderer->canvases[slot].in_use) return;
		target_texture = renderer->canvases[slot].texture;
	}
	
	if (!target_texture) return;
	
	SDL_GPUColorTargetInfo color_target = {
		.texture = target_texture,
		.mip_level = 0,
		.layer_or_depth_plane = 0,
		.clear_color = { 0.0f, 0.0f, 0.0f, 1.0f },
		.load_op = clear ? SDL_GPU_LOADOP_CLEAR : SDL_GPU_LOADOP_LOAD,
		.store_op = SDL_GPU_STOREOP_STORE,
		.resolve_texture = NULL,
		.resolve_mip_level = 0,
		.resolve_layer = 0,
		.cycle = false,
		.cycle_resolve_texture = false
	};
	
	renderer->render_pass = SDL_BeginGPURenderPass(renderer->cmd_buffer, &color_target, 1, NULL);
}

//--------------------------------------------------------------------------------------------------
// Drawing state management
//--------------------------------------------------------------------------------------------------

void cfr_push_color(CFR_Renderer* renderer, SDL_FColor color)
{
	STACK_PUSH(color, SDL_FColor, color);
}

SDL_FColor cfr_pop_color(CFR_Renderer* renderer)
{
	SDL_FColor default_color = { 1, 1, 1, 1 };
	return STACK_POP(color, SDL_FColor, default_color);
}

SDL_FColor cfr_peek_color(CFR_Renderer* renderer)
{
	SDL_FColor default_color = { 1, 1, 1, 1 };
	return STACK_PEEK(color, SDL_FColor, default_color);
}

void cfr_push_layer(CFR_Renderer* renderer, int layer)
{
	STACK_PUSH(layer, int, layer);
}

int cfr_pop_layer(CFR_Renderer* renderer)
{
	return STACK_POP(layer, int, 0);
}

int cfr_peek_layer(CFR_Renderer* renderer)
{
	return STACK_PEEK(layer, int, 0);
}

void cfr_push_antialias(CFR_Renderer* renderer, bool enabled)
{
	STACK_PUSH(antialias, bool, enabled);
}

bool cfr_pop_antialias(CFR_Renderer* renderer)
{
	return STACK_POP(antialias, bool, true);
}

bool cfr_peek_antialias(CFR_Renderer* renderer)
{
	return STACK_PEEK(antialias, bool, true);
}

void cfr_push_antialias_scale(CFR_Renderer* renderer, float scale)
{
	STACK_PUSH(antialias_scale, float, scale);
}

float cfr_pop_antialias_scale(CFR_Renderer* renderer)
{
	return STACK_POP(antialias_scale, float, 1.5f);
}

float cfr_peek_antialias_scale(CFR_Renderer* renderer)
{
	return STACK_PEEK(antialias_scale, float, 1.5f);
}

void cfr_push_viewport(CFR_Renderer* renderer, SDL_FRect viewport)
{
	STACK_PUSH(viewport, SDL_FRect, viewport);
}

SDL_FRect cfr_pop_viewport(CFR_Renderer* renderer)
{
	SDL_FRect default_viewport = { 0, 0, (float)renderer->width, (float)renderer->height };
	return STACK_POP(viewport, SDL_FRect, default_viewport);
}

SDL_FRect cfr_peek_viewport(CFR_Renderer* renderer)
{
	SDL_FRect default_viewport = { 0, 0, (float)renderer->width, (float)renderer->height };
	return STACK_PEEK(viewport, SDL_FRect, default_viewport);
}

void cfr_push_scissor(CFR_Renderer* renderer, SDL_FRect scissor)
{
	STACK_PUSH(scissor, SDL_FRect, scissor);
}

SDL_FRect cfr_pop_scissor(CFR_Renderer* renderer)
{
	SDL_FRect default_scissor = { 0, 0, -1, -1 };
	return STACK_POP(scissor, SDL_FRect, default_scissor);
}

SDL_FRect cfr_peek_scissor(CFR_Renderer* renderer)
{
	SDL_FRect default_scissor = { 0, 0, -1, -1 };
	return STACK_PEEK(scissor, SDL_FRect, default_scissor);
}

void cfr_push_blend_state(CFR_Renderer* renderer, SDL_GPUColorTargetBlendState blend)
{
	STACK_PUSH(blend, SDL_GPUColorTargetBlendState, blend);
}

SDL_GPUColorTargetBlendState cfr_pop_blend_state(CFR_Renderer* renderer)
{
	return STACK_POP(blend, SDL_GPUColorTargetBlendState, cfr_blend_state_alpha());
}

SDL_GPUColorTargetBlendState cfr_peek_blend_state(CFR_Renderer* renderer)
{
	return STACK_PEEK(blend, SDL_GPUColorTargetBlendState, cfr_blend_state_alpha());
}

//--------------------------------------------------------------------------------------------------
// Transformation stack
//--------------------------------------------------------------------------------------------------

void cfr_push_transform(CFR_Renderer* renderer, CFR_M3x2 transform)
{
	STACK_PUSH(transform, CFR_M3x2, transform);
	renderer->mvp = cfr_mul_m3x2(renderer->projection, transform);
}

CFR_M3x2 cfr_pop_transform(CFR_Renderer* renderer)
{
	CFR_M3x2 result = STACK_POP(transform, CFR_M3x2, cfr_make_identity());
	renderer->mvp = cfr_mul_m3x2(renderer->projection, STACK_PEEK(transform, CFR_M3x2, cfr_make_identity()));
	return result;
}

CFR_M3x2 cfr_peek_transform(CFR_Renderer* renderer)
{
	return STACK_PEEK(transform, CFR_M3x2, cfr_make_identity());
}

void cfr_translate(CFR_Renderer* renderer, float x, float y)
{
	CFR_M3x2 current = cfr_peek_transform(renderer);
	CFR_M3x2 translation = cfr_make_translation(x, y);
	cfr_pop_transform(renderer);
	cfr_push_transform(renderer, cfr_mul_m3x2(current, translation));
}

void cfr_rotate(CFR_Renderer* renderer, float radians)
{
	CFR_M3x2 current = cfr_peek_transform(renderer);
	CFR_M3x2 rotation = cfr_make_rotation(radians);
	cfr_pop_transform(renderer);
	cfr_push_transform(renderer, cfr_mul_m3x2(current, rotation));
}

void cfr_scale(CFR_Renderer* renderer, float sx, float sy)
{
	CFR_M3x2 current = cfr_peek_transform(renderer);
	CFR_M3x2 scale = cfr_make_scale(sx, sy);
	cfr_pop_transform(renderer);
	cfr_push_transform(renderer, cfr_mul_m3x2(current, scale));
}

CFR_V2 cfr_transform_point(CFR_Renderer* renderer, CFR_V2 point)
{
	return cfr_mul_m3x2_v2(cfr_peek_transform(renderer), point);
}

//--------------------------------------------------------------------------------------------------
// High-level drawing functions - Sprites
//--------------------------------------------------------------------------------------------------

void cfr_draw_sprite(CFR_Renderer* renderer, const CFR_Sprite* sprite)
{
	if (!renderer || !sprite) return;
	
	// Create a sprite batch item
	spritebatch_sprite_t s = {0};
	s.image_id = sprite->texture.id;
	s.w = sprite->w;
	s.h = sprite->h;
	s.minx = 0.0f;
	s.miny = 0.0f;
	s.maxx = 1.0f;
	s.maxy = 1.0f;
	
	// Transform quad corners
	CFR_V2 quad[4] = {
		{ -0.5f,  0.5f },
		{  0.5f,  0.5f },
		{  0.5f, -0.5f },
		{ -0.5f, -0.5f }
	};
	
	CFR_V2 scale = cfr_mul(cfr_v2(sprite->w, sprite->h), sprite->scale);
	
	for (int i = 0; i < 4; i++) {
		float x = quad[i].x * scale.x;
		float y = quad[i].y * scale.y;
		
		// Apply rotation
		float rx = sprite->transform.r.c * x - sprite->transform.r.s * y;
		float ry = sprite->transform.r.s * x + sprite->transform.r.c * y;
		
		// Apply translation
		quad[i].x = rx + sprite->transform.p.x + sprite->offset.x;
		quad[i].y = ry + sprite->transform.p.y + sprite->offset.y;
		
		// Transform by MVP
		quad[i] = cfr_mul_m3x2_v2(renderer->mvp, quad[i]);
	}
	
	s.geom.type = GEOM_TYPE_SPRITE;
	s.geom.shape[0] = quad[0];
	s.geom.shape[1] = quad[1];
	s.geom.shape[2] = quad[2];
	s.geom.shape[3] = quad[3];
	s.geom.color = cfr_peek_color(renderer);
	s.geom.alpha = sprite->opacity;
	
	spritebatch_push(&renderer->sprite_batch, s);
}

//--------------------------------------------------------------------------------------------------
// High-level drawing functions - Shapes (stub implementations)
//--------------------------------------------------------------------------------------------------

void cfr_draw_line(CFR_Renderer* renderer, CFR_V2 p0, CFR_V2 p1, float thickness)
{
	// Stub - would implement line drawing
}

void cfr_draw_circle(CFR_Renderer* renderer, CFR_Circle circle, float thickness)
{
	// Stub - would implement circle drawing
}

void cfr_draw_circle_fill(CFR_Renderer* renderer, CFR_Circle circle)
{
	// Stub - would implement filled circle drawing
}

void cfr_draw_quad(CFR_Renderer* renderer, CFR_Aabb bb, float thickness, float chubbiness)
{
	// Stub - would implement quad drawing
}

void cfr_draw_quad_fill(CFR_Renderer* renderer, CFR_Aabb bb, float chubbiness)
{
	// Stub - would implement filled quad drawing
}

void cfr_draw_quad2(CFR_Renderer* renderer, CFR_V2 p0, CFR_V2 p1, CFR_V2 p2, CFR_V2 p3, float thickness, float chubbiness)
{
	// Stub - would implement quad drawing
}

void cfr_draw_quad_fill2(CFR_Renderer* renderer, CFR_V2 p0, CFR_V2 p1, CFR_V2 p2, CFR_V2 p3, float chubbiness)
{
	// Stub - would implement filled quad drawing
}

void cfr_draw_box_rounded(CFR_Renderer* renderer, CFR_Aabb bb, float thickness, float radius)
{
	// Stub - would implement rounded box drawing
}

void cfr_draw_box_rounded_fill(CFR_Renderer* renderer, CFR_Aabb bb, float radius)
{
	// Stub - would implement filled rounded box drawing
}

void cfr_draw_capsule(CFR_Renderer* renderer, CFR_V2 p0, CFR_V2 p1, float r, float thickness)
{
	// Stub - would implement capsule drawing
}

void cfr_draw_capsule_fill(CFR_Renderer* renderer, CFR_V2 p0, CFR_V2 p1, float r)
{
	// Stub - would implement filled capsule drawing
}

void cfr_draw_tri(CFR_Renderer* renderer, CFR_V2 p0, CFR_V2 p1, CFR_V2 p2, float thickness, float chubbiness)
{
	// Stub - would implement triangle drawing
}

void cfr_draw_tri_fill(CFR_Renderer* renderer, CFR_V2 p0, CFR_V2 p1, CFR_V2 p2, float chubbiness)
{
	// Stub - would implement filled triangle drawing
}

void cfr_draw_polygon(CFR_Renderer* renderer, CFR_V2* points, int count, float thickness)
{
	// Stub - would implement polygon drawing
}

void cfr_draw_polygon_fill(CFR_Renderer* renderer, CFR_V2* points, int count)
{
	// Stub - would implement filled polygon drawing
}

void cfr_draw_bezier_line(CFR_Renderer* renderer, CFR_V2 a, CFR_V2 c0, CFR_V2 b, float thickness)
{
	// Stub - would implement bezier line drawing
}

void cfr_draw_bezier_line2(CFR_Renderer* renderer, CFR_V2 a, CFR_V2 c0, CFR_V2 c1, CFR_V2 b, float thickness)
{
	// Stub - would implement bezier line drawing
}
