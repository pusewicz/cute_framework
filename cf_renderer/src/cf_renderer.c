/*
	CF Renderer - A 2D renderer based on SDL3 GPU API
	Extracted from Cute Framework
	Copyright (C) 2024 Randy Gaul https://randygaul.github.io/
	
	This software is dual-licensed with zlib or Unlicense, check LICENSE.txt for more info
*/

#include "../include/cf_renderer.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>

//--------------------------------------------------------------------------------------------------
// Internal structures
//--------------------------------------------------------------------------------------------------

#define CFR_MAX_TEXTURES 1024
#define CFR_MAX_CANVASES 64
#define CFR_MAX_MESHES 128
#define CFR_MAX_SHADERS 64
#define CFR_MAX_MATERIALS 128
#define CFR_MAX_VERTEX_ATTRIBUTES 16
#define CFR_MAX_UNIFORM_BLOCKS 4

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

typedef struct CFR_MeshInternal {
	SDL_GPUBuffer* vertex_buffer;
	SDL_GPUTransferBuffer* transfer_buffer;
	int vertex_buffer_size;
	int vertex_stride;
	int attribute_count;
	CFR_VertexAttribute attributes[CFR_MAX_VERTEX_ATTRIBUTES];
	bool in_use;
} CFR_MeshInternal;

typedef struct CFR_ShaderInternal {
	SDL_GPUShader* vertex_shader;
	SDL_GPUShader* fragment_shader;
	bool in_use;
} CFR_ShaderInternal;

typedef struct CFR_MaterialInternal {
	CFR_RenderState render_state;
	bool in_use;
} CFR_MaterialInternal;

struct CFR_Renderer {
	SDL_GPUDevice* device;
	SDL_Window* window;
	SDL_GPUCommandBuffer* cmd_buffer;
	SDL_GPURenderPass* render_pass;
	SDL_GPUTexture* swapchain_texture;
	
	// Resource pools
	CFR_TextureInternal textures[CFR_MAX_TEXTURES];
	CFR_CanvasInternal canvases[CFR_MAX_CANVASES];
	CFR_MeshInternal meshes[CFR_MAX_MESHES];
	CFR_ShaderInternal shaders[CFR_MAX_SHADERS];
	CFR_MaterialInternal materials[CFR_MAX_MATERIALS];
	
	// Current state
	CFR_CanvasInternal* current_canvas;
	CFR_MeshInternal* current_mesh;
	SDL_GPUGraphicsPipeline* current_pipeline;
	
	bool frame_begun;
};

//--------------------------------------------------------------------------------------------------
// Utility functions
//--------------------------------------------------------------------------------------------------

static SDL_GPUBlendFactor cfr_to_sdl_blend_factor(CFR_BlendFactor factor)
{
	switch (factor) {
		case CFR_BLENDFACTOR_ZERO: return SDL_GPU_BLENDFACTOR_ZERO;
		case CFR_BLENDFACTOR_ONE: return SDL_GPU_BLENDFACTOR_ONE;
		case CFR_BLENDFACTOR_SRC_COLOR: return SDL_GPU_BLENDFACTOR_SRC_COLOR;
		case CFR_BLENDFACTOR_ONE_MINUS_SRC_COLOR: return SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_COLOR;
		case CFR_BLENDFACTOR_DST_COLOR: return SDL_GPU_BLENDFACTOR_DST_COLOR;
		case CFR_BLENDFACTOR_ONE_MINUS_DST_COLOR: return SDL_GPU_BLENDFACTOR_ONE_MINUS_DST_COLOR;
		case CFR_BLENDFACTOR_SRC_ALPHA: return SDL_GPU_BLENDFACTOR_SRC_ALPHA;
		case CFR_BLENDFACTOR_ONE_MINUS_SRC_ALPHA: return SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
		case CFR_BLENDFACTOR_DST_ALPHA: return SDL_GPU_BLENDFACTOR_DST_ALPHA;
		case CFR_BLENDFACTOR_ONE_MINUS_DST_ALPHA: return SDL_GPU_BLENDFACTOR_ONE_MINUS_DST_ALPHA;
		default: return SDL_GPU_BLENDFACTOR_ONE;
	}
}

static SDL_GPUBlendOp cfr_to_sdl_blend_op(CFR_BlendOp op)
{
	switch (op) {
		case CFR_BLEND_OP_ADD: return SDL_GPU_BLENDOP_ADD;
		case CFR_BLEND_OP_SUBTRACT: return SDL_GPU_BLENDOP_SUBTRACT;
		case CFR_BLEND_OP_REVERSE_SUBTRACT: return SDL_GPU_BLENDOP_REVERSE_SUBTRACT;
		case CFR_BLEND_OP_MIN: return SDL_GPU_BLENDOP_MIN;
		case CFR_BLEND_OP_MAX: return SDL_GPU_BLENDOP_MAX;
		default: return SDL_GPU_BLENDOP_ADD;
	}
}

static SDL_GPUVertexElementFormat cfr_to_sdl_vertex_format(CFR_VertexFormat format)
{
	switch (format) {
		case CFR_VERTEX_FORMAT_FLOAT: return SDL_GPU_VERTEXELEMENTFORMAT_FLOAT;
		case CFR_VERTEX_FORMAT_FLOAT2: return SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2;
		case CFR_VERTEX_FORMAT_FLOAT3: return SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3;
		case CFR_VERTEX_FORMAT_FLOAT4: return SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4;
		case CFR_VERTEX_FORMAT_UBYTE4: return SDL_GPU_VERTEXELEMENTFORMAT_UBYTE4;
		case CFR_VERTEX_FORMAT_UBYTE4_NORM: return SDL_GPU_VERTEXELEMENTFORMAT_UBYTE4_NORM;
		case CFR_VERTEX_FORMAT_INT: return SDL_GPU_VERTEXELEMENTFORMAT_INT;
		case CFR_VERTEX_FORMAT_UINT: return SDL_GPU_VERTEXELEMENTFORMAT_UINT;
		default: return SDL_GPU_VERTEXELEMENTFORMAT_FLOAT;
	}
}

//--------------------------------------------------------------------------------------------------
// Renderer lifecycle
//--------------------------------------------------------------------------------------------------

CFR_Renderer* cfr_create_renderer(SDL_GPUDevice* device, SDL_Window* window)
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
	renderer->frame_begun = false;
	
	return renderer;
}

void cfr_destroy_renderer(CFR_Renderer* renderer)
{
	if (!renderer) {
		return;
	}
	
	// Clean up all resources
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
	
	for (int i = 0; i < CFR_MAX_MESHES; i++) {
		if (renderer->meshes[i].in_use) {
			if (renderer->meshes[i].vertex_buffer) {
				SDL_ReleaseGPUBuffer(renderer->device, renderer->meshes[i].vertex_buffer);
			}
			if (renderer->meshes[i].transfer_buffer) {
				SDL_ReleaseGPUTransferBuffer(renderer->device, renderer->meshes[i].transfer_buffer);
			}
		}
	}
	
	for (int i = 0; i < CFR_MAX_SHADERS; i++) {
		if (renderer->shaders[i].in_use) {
			if (renderer->shaders[i].vertex_shader) {
				SDL_ReleaseGPUShader(renderer->device, renderer->shaders[i].vertex_shader);
			}
			if (renderer->shaders[i].fragment_shader) {
				SDL_ReleaseGPUShader(renderer->device, renderer->shaders[i].fragment_shader);
			}
		}
	}
	
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

//--------------------------------------------------------------------------------------------------
// Texture management
//--------------------------------------------------------------------------------------------------

CFR_Texture cfr_make_texture(CFR_Renderer* renderer, CFR_TextureParams params)
{
	CFR_Texture result = { 0 };
	if (!renderer) {
		return result;
	}
	
	// Find free slot
	int slot = -1;
	for (int i = 0; i < CFR_MAX_TEXTURES; i++) {
		if (!renderer->textures[i].in_use) {
			slot = i;
			break;
		}
	}
	
	if (slot == -1) {
		return result;
	}
	
	CFR_TextureInternal* tex = &renderer->textures[slot];
	
	SDL_GPUTextureCreateInfo create_info = {
		.type = SDL_GPU_TEXTURETYPE_2D,
		.format = params.format,
		.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER,
		.width = params.width,
		.height = params.height,
		.layer_count_or_depth = 1,
		.num_levels = 1,
		.sample_count = SDL_GPU_SAMPLECOUNT_1,
		.props = 0
	};
	
	tex->texture = SDL_CreateGPUTexture(renderer->device, &create_info);
	if (!tex->texture) {
		return result;
	}
	
	SDL_GPUSamplerCreateInfo sampler_info = {
		.min_filter = params.filter == CFR_FILTER_LINEAR ? SDL_GPU_FILTER_LINEAR : SDL_GPU_FILTER_NEAREST,
		.mag_filter = params.filter == CFR_FILTER_LINEAR ? SDL_GPU_FILTER_LINEAR : SDL_GPU_FILTER_NEAREST,
		.mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_NEAREST,
		.address_mode_u = params.wrap_u == CFR_WRAP_REPEAT ? SDL_GPU_SAMPLERADDRESSMODE_REPEAT : SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
		.address_mode_v = params.wrap_v == CFR_WRAP_REPEAT ? SDL_GPU_SAMPLERADDRESSMODE_REPEAT : SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
		.address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
		.mip_lod_bias = 0.0f,
		.max_anisotropy = 1.0f,
		.compare_op = SDL_GPU_COMPAREOP_NEVER,
		.min_lod = 0.0f,
		.max_lod = 1000.0f,
		.enable_anisotropy = false,
		.enable_compare = false,
		.props = 0
	};
	
	tex->sampler = SDL_CreateGPUSampler(renderer->device, &sampler_info);
	if (!tex->sampler) {
		SDL_ReleaseGPUTexture(renderer->device, tex->texture);
		tex->texture = NULL;
		return result;
	}
	
	tex->width = params.width;
	tex->height = params.height;
	tex->format = params.format;
	tex->in_use = true;
	
	result.id = (uint64_t)(slot + 1);
	return result;
}

void cfr_texture_update(CFR_Renderer* renderer, CFR_Texture texture, void* pixels, int size)
{
	if (!renderer || !texture.id || !pixels) {
		return;
	}
	
	int slot = (int)(texture.id - 1);
	if (slot < 0 || slot >= CFR_MAX_TEXTURES || !renderer->textures[slot].in_use) {
		return;
	}
	
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
	
	if (!tex->transfer_buffer) {
		return;
	}
	
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
	if (!renderer || !texture.id) {
		return;
	}
	
	int slot = (int)(texture.id - 1);
	if (slot < 0 || slot >= CFR_MAX_TEXTURES || !renderer->textures[slot].in_use) {
		return;
	}
	
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

//--------------------------------------------------------------------------------------------------
// Canvas management
//--------------------------------------------------------------------------------------------------

CFR_Canvas cfr_make_canvas(CFR_Renderer* renderer, int width, int height)
{
	CFR_Canvas result = { 0 };
	if (!renderer) {
		return result;
	}
	
	// Find free slot
	int slot = -1;
	for (int i = 0; i < CFR_MAX_CANVASES; i++) {
		if (!renderer->canvases[i].in_use) {
			slot = i;
			break;
		}
	}
	
	if (slot == -1) {
		return result;
	}
	
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
	if (!canvas->texture) {
		return result;
	}
	
	SDL_GPUSamplerCreateInfo sampler_info = {
		.min_filter = SDL_GPU_FILTER_LINEAR,
		.mag_filter = SDL_GPU_FILTER_LINEAR,
		.mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_NEAREST,
		.address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
		.address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
		.address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
		.mip_lod_bias = 0.0f,
		.max_anisotropy = 1.0f,
		.compare_op = SDL_GPU_COMPAREOP_NEVER,
		.min_lod = 0.0f,
		.max_lod = 1000.0f,
		.enable_anisotropy = false,
		.enable_compare = false,
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

CFR_Texture cfr_canvas_get_texture(CFR_Renderer* renderer, CFR_Canvas canvas)
{
	CFR_Texture result = { 0 };
	if (!renderer || !canvas.id) {
		return result;
	}
	
	// For canvas texture, we return the canvas ID directly
	// This is a simplification - in a real implementation you might want separate texture handles
	result.id = canvas.id;
	return result;
}

void cfr_destroy_canvas(CFR_Renderer* renderer, CFR_Canvas canvas)
{
	if (!renderer || !canvas.id) {
		return;
	}
	
	int slot = (int)(canvas.id - 1);
	if (slot < 0 || slot >= CFR_MAX_CANVASES || !renderer->canvases[slot].in_use) {
		return;
	}
	
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

void cfr_apply_canvas(CFR_Renderer* renderer, CFR_Canvas canvas, bool clear)
{
	if (!renderer) {
		return;
	}
	
	// End current render pass if active
	if (renderer->render_pass) {
		SDL_EndGPURenderPass(renderer->render_pass);
		renderer->render_pass = NULL;
	}
	
	SDL_GPUTexture* target_texture;
	
	if (canvas.id == 0) {
		// Render to swapchain
		if (!renderer->cmd_buffer) {
			return;
		}
		
		if (!SDL_WaitAndAcquireGPUSwapchainTexture(renderer->cmd_buffer, renderer->window, &target_texture, NULL, NULL)) {
			return;
		}
		renderer->swapchain_texture = target_texture;
		renderer->current_canvas = NULL;
	} else {
		// Render to canvas
		int slot = (int)(canvas.id - 1);
		if (slot < 0 || slot >= CFR_MAX_CANVASES || !renderer->canvases[slot].in_use) {
			return;
		}
		target_texture = renderer->canvases[slot].texture;
		renderer->current_canvas = &renderer->canvases[slot];
	}
	
	if (!target_texture) {
		return;
	}
	
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

void cfr_clear_canvas(CFR_Renderer* renderer, CFR_Color color)
{
	// Clear is handled by cfr_apply_canvas
	// This is a no-op in this implementation
	(void)renderer;
	(void)color;
}

//--------------------------------------------------------------------------------------------------
// Mesh management
//--------------------------------------------------------------------------------------------------

CFR_Mesh cfr_make_mesh(CFR_Renderer* renderer, int vertex_buffer_size,
                       CFR_VertexAttribute* attributes, int attribute_count, int vertex_stride)
{
	CFR_Mesh result = { 0 };
	if (!renderer || !attributes || attribute_count == 0 || attribute_count > CFR_MAX_VERTEX_ATTRIBUTES) {
		return result;
	}
	
	// Find free slot
	int slot = -1;
	for (int i = 0; i < CFR_MAX_MESHES; i++) {
		if (!renderer->meshes[i].in_use) {
			slot = i;
			break;
		}
	}
	
	if (slot == -1) {
		return result;
	}
	
	CFR_MeshInternal* mesh = &renderer->meshes[slot];
	
	SDL_GPUBufferCreateInfo buffer_info = {
		.usage = SDL_GPU_BUFFERUSAGE_VERTEX,
		.size = vertex_buffer_size,
		.props = 0
	};
	
	mesh->vertex_buffer = SDL_CreateGPUBuffer(renderer->device, &buffer_info);
	if (!mesh->vertex_buffer) {
		return result;
	}
	
	SDL_GPUTransferBufferCreateInfo transfer_info = {
		.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
		.size = vertex_buffer_size,
		.props = 0
	};
	
	mesh->transfer_buffer = SDL_CreateGPUTransferBuffer(renderer->device, &transfer_info);
	if (!mesh->transfer_buffer) {
		SDL_ReleaseGPUBuffer(renderer->device, mesh->vertex_buffer);
		mesh->vertex_buffer = NULL;
		return result;
	}
	
	mesh->vertex_buffer_size = vertex_buffer_size;
	mesh->vertex_stride = vertex_stride;
	mesh->attribute_count = attribute_count;
	for (int i = 0; i < attribute_count; i++) {
		mesh->attributes[i] = attributes[i];
	}
	mesh->in_use = true;
	
	result.id = (uint64_t)(slot + 1);
	return result;
}

void cfr_mesh_update_vertex_data(CFR_Renderer* renderer, CFR_Mesh mesh, void* vertices, int vertex_count)
{
	if (!renderer || !mesh.id || !vertices) {
		return;
	}
	
	int slot = (int)(mesh.id - 1);
	if (slot < 0 || slot >= CFR_MAX_MESHES || !renderer->meshes[slot].in_use) {
		return;
	}
	
	CFR_MeshInternal* m = &renderer->meshes[slot];
	int size = vertex_count * m->vertex_stride;
	
	if (size > m->vertex_buffer_size) {
		return;
	}
	
	// Map and copy data
	void* mapped = SDL_MapGPUTransferBuffer(renderer->device, m->transfer_buffer, false);
	if (mapped) {
		memcpy(mapped, vertices, size);
		SDL_UnmapGPUTransferBuffer(renderer->device, m->transfer_buffer);
	}
	
	// Upload to buffer
	if (renderer->cmd_buffer) {
		SDL_GPUCopyPass* copy_pass = SDL_BeginGPUCopyPass(renderer->cmd_buffer);
		if (copy_pass) {
			SDL_GPUTransferBufferLocation src = {
				.transfer_buffer = m->transfer_buffer,
				.offset = 0
			};
			
			SDL_GPUBufferRegion dst = {
				.buffer = m->vertex_buffer,
				.offset = 0,
				.size = size
			};
			
			SDL_UploadToGPUBuffer(copy_pass, &src, &dst, false);
			SDL_EndGPUCopyPass(copy_pass);
		}
	}
}

void cfr_destroy_mesh(CFR_Renderer* renderer, CFR_Mesh mesh)
{
	if (!renderer || !mesh.id) {
		return;
	}
	
	int slot = (int)(mesh.id - 1);
	if (slot < 0 || slot >= CFR_MAX_MESHES || !renderer->meshes[slot].in_use) {
		return;
	}
	
	CFR_MeshInternal* m = &renderer->meshes[slot];
	
	if (m->vertex_buffer) {
		SDL_ReleaseGPUBuffer(renderer->device, m->vertex_buffer);
		m->vertex_buffer = NULL;
	}
	
	if (m->transfer_buffer) {
		SDL_ReleaseGPUTransferBuffer(renderer->device, m->transfer_buffer);
		m->transfer_buffer = NULL;
	}
	
	m->in_use = false;
}

void cfr_apply_mesh(CFR_Renderer* renderer, CFR_Mesh mesh)
{
	if (!renderer || !mesh.id) {
		return;
	}
	
	int slot = (int)(mesh.id - 1);
	if (slot < 0 || slot >= CFR_MAX_MESHES || !renderer->meshes[slot].in_use) {
		return;
	}
	
	renderer->current_mesh = &renderer->meshes[slot];
}

//--------------------------------------------------------------------------------------------------
// Shader management
//--------------------------------------------------------------------------------------------------

CFR_Shader cfr_make_shader(CFR_Renderer* renderer,
                           const void* vs_bytecode, size_t vs_size,
                           const void* fs_bytecode, size_t fs_size)
{
	CFR_Shader result = { 0 };
	if (!renderer || !vs_bytecode || !fs_bytecode) {
		return result;
	}
	
	// Find free slot
	int slot = -1;
	for (int i = 0; i < CFR_MAX_SHADERS; i++) {
		if (!renderer->shaders[i].in_use) {
			slot = i;
			break;
		}
	}
	
	if (slot == -1) {
		return result;
	}
	
	CFR_ShaderInternal* shader = &renderer->shaders[slot];
	
	SDL_GPUShaderCreateInfo vs_info = {
		.code_size = vs_size,
		.code = (const uint8_t*)vs_bytecode,
		.entrypoint = "main",
		.format = SDL_GPU_SHADERFORMAT_SPIRV,
		.stage = SDL_GPU_SHADERSTAGE_VERTEX,
		.num_samplers = 0,
		.num_storage_textures = 0,
		.num_storage_buffers = 0,
		.num_uniform_buffers = 0,
		.props = 0
	};
	
	shader->vertex_shader = SDL_CreateGPUShader(renderer->device, &vs_info);
	if (!shader->vertex_shader) {
		return result;
	}
	
	SDL_GPUShaderCreateInfo fs_info = {
		.code_size = fs_size,
		.code = (const uint8_t*)fs_bytecode,
		.entrypoint = "main",
		.format = SDL_GPU_SHADERFORMAT_SPIRV,
		.stage = SDL_GPU_SHADERSTAGE_FRAGMENT,
		.num_samplers = 1,
		.num_storage_textures = 0,
		.num_storage_buffers = 0,
		.num_uniform_buffers = 0,
		.props = 0
	};
	
	shader->fragment_shader = SDL_CreateGPUShader(renderer->device, &fs_info);
	if (!shader->fragment_shader) {
		SDL_ReleaseGPUShader(renderer->device, shader->vertex_shader);
		shader->vertex_shader = NULL;
		return result;
	}
	
	shader->in_use = true;
	
	result.id = (uint64_t)(slot + 1);
	return result;
}

void cfr_destroy_shader(CFR_Renderer* renderer, CFR_Shader shader)
{
	if (!renderer || !shader.id) {
		return;
	}
	
	int slot = (int)(shader.id - 1);
	if (slot < 0 || slot >= CFR_MAX_SHADERS || !renderer->shaders[slot].in_use) {
		return;
	}
	
	CFR_ShaderInternal* s = &renderer->shaders[slot];
	
	if (s->vertex_shader) {
		SDL_ReleaseGPUShader(renderer->device, s->vertex_shader);
		s->vertex_shader = NULL;
	}
	
	if (s->fragment_shader) {
		SDL_ReleaseGPUShader(renderer->device, s->fragment_shader);
		s->fragment_shader = NULL;
	}
	
	s->in_use = false;
}

//--------------------------------------------------------------------------------------------------
// Material management
//--------------------------------------------------------------------------------------------------

CFR_Material cfr_make_material(CFR_Renderer* renderer)
{
	CFR_Material result = { 0 };
	if (!renderer) {
		return result;
	}
	
	// Find free slot
	int slot = -1;
	for (int i = 0; i < CFR_MAX_MATERIALS; i++) {
		if (!renderer->materials[i].in_use) {
			slot = i;
			break;
		}
	}
	
	if (slot == -1) {
		return result;
	}
	
	CFR_MaterialInternal* material = &renderer->materials[slot];
	material->render_state = cfr_render_state_defaults();
	material->in_use = true;
	
	result.id = (uint64_t)(slot + 1);
	return result;
}

void cfr_material_set_texture(CFR_Renderer* renderer, CFR_Material material,
                               const char* name, CFR_Texture texture)
{
	// Texture binding is handled in cfr_apply_shader
	// This is a simplified implementation
	(void)renderer;
	(void)material;
	(void)name;
	(void)texture;
}

void cfr_material_set_uniform(CFR_Renderer* renderer, CFR_Material material,
                               const char* name, void* data, CFR_UniformType type, int array_length)
{
	// Uniform binding is handled in cfr_apply_shader
	// This is a simplified implementation
	(void)renderer;
	(void)material;
	(void)name;
	(void)data;
	(void)type;
	(void)array_length;
}

void cfr_material_set_render_state(CFR_Renderer* renderer, CFR_Material material,
                                    CFR_RenderState render_state)
{
	if (!renderer || !material.id) {
		return;
	}
	
	int slot = (int)(material.id - 1);
	if (slot < 0 || slot >= CFR_MAX_MATERIALS || !renderer->materials[slot].in_use) {
		return;
	}
	
	renderer->materials[slot].render_state = render_state;
}

void cfr_destroy_material(CFR_Renderer* renderer, CFR_Material material)
{
	if (!renderer || !material.id) {
		return;
	}
	
	int slot = (int)(material.id - 1);
	if (slot < 0 || slot >= CFR_MAX_MATERIALS || !renderer->materials[slot].in_use) {
		return;
	}
	
	renderer->materials[slot].in_use = false;
}

//--------------------------------------------------------------------------------------------------
// Drawing
//--------------------------------------------------------------------------------------------------

void cfr_apply_shader(CFR_Renderer* renderer, CFR_Shader shader, CFR_Material material)
{
	if (!renderer || !shader.id || !material.id || !renderer->render_pass) {
		return;
	}
	
	int shader_slot = (int)(shader.id - 1);
	int material_slot = (int)(material.id - 1);
	
	if (shader_slot < 0 || shader_slot >= CFR_MAX_SHADERS || !renderer->shaders[shader_slot].in_use) {
		return;
	}
	
	if (material_slot < 0 || material_slot >= CFR_MAX_MATERIALS || !renderer->materials[material_slot].in_use) {
		return;
	}
	
	CFR_ShaderInternal* s = &renderer->shaders[shader_slot];
	CFR_MaterialInternal* m = &renderer->materials[material_slot];
	
	if (!renderer->current_mesh) {
		return;
	}
	
	// Build pipeline
	SDL_GPUGraphicsPipelineCreateInfo pipeline_info = { 0 };
	
	// Vertex input state
	SDL_GPUVertexBufferDescription vertex_buffer_desc = {
		.slot = 0,
		.pitch = renderer->current_mesh->vertex_stride,
		.input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX,
		.instance_step_rate = 0
	};
	
	SDL_GPUVertexAttribute vertex_attributes[CFR_MAX_VERTEX_ATTRIBUTES];
	for (int i = 0; i < renderer->current_mesh->attribute_count; i++) {
		vertex_attributes[i].location = i;
		vertex_attributes[i].buffer_slot = 0;
		vertex_attributes[i].format = cfr_to_sdl_vertex_format(renderer->current_mesh->attributes[i].format);
		vertex_attributes[i].offset = renderer->current_mesh->attributes[i].offset;
	}
	
	SDL_GPUVertexInputState vertex_input_state = {
		.vertex_buffer_descriptions = &vertex_buffer_desc,
		.num_vertex_buffers = 1,
		.vertex_attributes = vertex_attributes,
		.num_vertex_attributes = renderer->current_mesh->attribute_count
	};
	
	pipeline_info.vertex_shader = s->vertex_shader;
	pipeline_info.fragment_shader = s->fragment_shader;
	pipeline_info.vertex_input_state = vertex_input_state;
	pipeline_info.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
	
	// Rasterizer state
	SDL_GPURasterizerState rasterizer_state = {
		.fill_mode = SDL_GPU_FILLMODE_FILL,
		.cull_mode = SDL_GPU_CULLMODE_NONE,
		.front_face = SDL_GPU_FRONTFACE_COUNTER_CLOCKWISE,
		.depth_bias_constant_factor = 0.0f,
		.depth_bias_clamp = 0.0f,
		.depth_bias_slope_factor = 0.0f,
		.enable_depth_bias = false,
		.enable_depth_clip = false
	};
	pipeline_info.rasterizer_state = rasterizer_state;
	
	// Multisample state
	SDL_GPUMultisampleState multisample_state = {
		.sample_count = SDL_GPU_SAMPLECOUNT_1,
		.sample_mask = 0xFFFFFFFF,
		.enable_mask = false
	};
	pipeline_info.multisample_state = multisample_state;
	
	// Depth-stencil state
	SDL_GPUDepthStencilState depth_stencil_state = {
		.compare_op = SDL_GPU_COMPAREOP_ALWAYS,
		.back_stencil_state = { 0 },
		.front_stencil_state = { 0 },
		.compare_mask = 0,
		.write_mask = 0,
		.enable_depth_test = false,
		.enable_depth_write = false,
		.enable_stencil_test = false
	};
	pipeline_info.depth_stencil_state = depth_stencil_state;
	
	// Blend state
	SDL_GPUColorTargetBlendState blend_state = {
		.src_color_blendfactor = cfr_to_sdl_blend_factor(m->render_state.blend.rgb_src_blend_factor),
		.dst_color_blendfactor = cfr_to_sdl_blend_factor(m->render_state.blend.rgb_dst_blend_factor),
		.color_blend_op = cfr_to_sdl_blend_op(m->render_state.blend.rgb_op),
		.src_alpha_blendfactor = cfr_to_sdl_blend_factor(m->render_state.blend.alpha_src_blend_factor),
		.dst_alpha_blendfactor = cfr_to_sdl_blend_factor(m->render_state.blend.alpha_dst_blend_factor),
		.alpha_blend_op = cfr_to_sdl_blend_op(m->render_state.blend.alpha_op),
		.color_write_mask = 0xF,
		.enable_blend = m->render_state.blend.enabled,
		.enable_color_write_mask = false
	};
	
	SDL_GPUColorTargetDescription color_target = {
		.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM,
		.blend_state = blend_state
	};
	
	pipeline_info.target_info.num_color_targets = 1;
	pipeline_info.target_info.color_target_descriptions = &color_target;
	pipeline_info.target_info.has_depth_stencil_target = false;
	pipeline_info.target_info.depth_stencil_format = SDL_GPU_TEXTUREFORMAT_D16_UNORM;
	
	renderer->current_pipeline = SDL_CreateGPUGraphicsPipeline(renderer->device, &pipeline_info);
	
	if (renderer->current_pipeline) {
		SDL_BindGPUGraphicsPipeline(renderer->render_pass, renderer->current_pipeline);
	}
}

void cfr_apply_viewport(CFR_Renderer* renderer, float x, float y, float w, float h)
{
	if (!renderer || !renderer->render_pass) {
		return;
	}
	
	SDL_GPUViewport viewport = {
		.x = x,
		.y = y,
		.w = w,
		.h = h,
		.min_depth = 0.0f,
		.max_depth = 1.0f
	};
	
	SDL_SetGPUViewport(renderer->render_pass, &viewport);
}

void cfr_apply_scissor(CFR_Renderer* renderer, float x, float y, float w, float h)
{
	if (!renderer || !renderer->render_pass) {
		return;
	}
	
	SDL_Rect scissor = {
		.x = (int)x,
		.y = (int)y,
		.w = (int)w,
		.h = (int)h
	};
	
	SDL_SetGPUScissor(renderer->render_pass, &scissor);
}

void cfr_draw_elements(CFR_Renderer* renderer)
{
	if (!renderer || !renderer->render_pass || !renderer->current_mesh) {
		return;
	}
	
	SDL_GPUBufferBinding vertex_binding = {
		.buffer = renderer->current_mesh->vertex_buffer,
		.offset = 0
	};
	
	SDL_BindGPUVertexBuffers(renderer->render_pass, 0, &vertex_binding, 1);
	
	// Draw vertices (this is simplified - would need vertex count tracking)
	SDL_DrawGPUPrimitives(renderer->render_pass, 6, 1, 0, 0);
	
	// Release pipeline after drawing
	if (renderer->current_pipeline) {
		SDL_ReleaseGPUGraphicsPipeline(renderer->device, renderer->current_pipeline);
		renderer->current_pipeline = NULL;
	}
}

//--------------------------------------------------------------------------------------------------
// Utility functions
//--------------------------------------------------------------------------------------------------

CFR_TextureParams cfr_texture_defaults(int width, int height)
{
	CFR_TextureParams params = {
		.width = width,
		.height = height,
		.filter = CFR_FILTER_LINEAR,
		.wrap_u = CFR_WRAP_CLAMP,
		.wrap_v = CFR_WRAP_CLAMP,
		.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM
	};
	return params;
}

CFR_RenderState cfr_render_state_defaults(void)
{
	CFR_RenderState state = { 0 };
	state.blend.enabled = true;
	state.blend.rgb_src_blend_factor = CFR_BLENDFACTOR_ONE;
	state.blend.rgb_dst_blend_factor = CFR_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
	state.blend.rgb_op = CFR_BLEND_OP_ADD;
	state.blend.alpha_src_blend_factor = CFR_BLENDFACTOR_ONE;
	state.blend.alpha_dst_blend_factor = CFR_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
	state.blend.alpha_op = CFR_BLEND_OP_ADD;
	state.depth_test_enabled = false;
	state.depth_write_enabled = false;
	state.stencil_enabled = false;
	return state;
}

CFR_Color cfr_make_color(uint8_t r, uint8_t g, uint8_t b, uint8_t a)
{
	CFR_Color color = {
		.r = r / 255.0f,
		.g = g / 255.0f,
		.b = b / 255.0f,
		.a = a / 255.0f
	};
	return color;
}

CFR_Color cfr_make_color_f(float r, float g, float b, float a)
{
	CFR_Color color = { .r = r, .g = g, .b = b, .a = a };
	return color;
}

CFR_Vec2 cfr_v2(float x, float y)
{
	CFR_Vec2 v = { .x = x, .y = y };
	return v;
}
