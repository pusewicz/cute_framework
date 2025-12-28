/*
	CF Renderer - A 2D renderer based on SDL3 GPU API
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

#ifdef __cplusplus
extern "C" {
#endif

//--------------------------------------------------------------------------------------------------
// Core types
//--------------------------------------------------------------------------------------------------

typedef struct CFR_Renderer CFR_Renderer;

typedef struct CFR_Color {
	float r, g, b, a;
} CFR_Color;

typedef struct CFR_Vec2 {
	float x, y;
} CFR_Vec2;

typedef struct CFR_Rect {
	float x, y, w, h;
} CFR_Rect;

typedef struct CFR_Aabb {
	CFR_Vec2 min, max;
} CFR_Aabb;

typedef struct CFR_Transform {
	CFR_Vec2 position;
	float rotation;
	CFR_Vec2 scale;
} CFR_Transform;

// Opaque handles for rendering resources
typedef struct { uint64_t id; } CFR_Texture;
typedef struct { uint64_t id; } CFR_Shader;
typedef struct { uint64_t id; } CFR_Canvas;
typedef struct { uint64_t id; } CFR_Mesh;
typedef struct { uint64_t id; } CFR_Material;

//--------------------------------------------------------------------------------------------------
// Pixel and texture types
//--------------------------------------------------------------------------------------------------

typedef struct CFR_Pixel {
	uint8_t r, g, b, a;
} CFR_Pixel;

typedef enum CFR_Filter {
	CFR_FILTER_NEAREST,
	CFR_FILTER_LINEAR
} CFR_Filter;

typedef enum CFR_TextureWrap {
	CFR_WRAP_REPEAT,
	CFR_WRAP_CLAMP,
	CFR_WRAP_MIRROR
} CFR_TextureWrap;

typedef struct CFR_TextureParams {
	int width;
	int height;
	CFR_Filter filter;
	CFR_TextureWrap wrap_u;
	CFR_TextureWrap wrap_v;
	SDL_GPUTextureFormat format;
} CFR_TextureParams;

//--------------------------------------------------------------------------------------------------
// Blend and render state
//--------------------------------------------------------------------------------------------------

typedef enum CFR_BlendFactor {
	CFR_BLENDFACTOR_ZERO,
	CFR_BLENDFACTOR_ONE,
	CFR_BLENDFACTOR_SRC_COLOR,
	CFR_BLENDFACTOR_ONE_MINUS_SRC_COLOR,
	CFR_BLENDFACTOR_DST_COLOR,
	CFR_BLENDFACTOR_ONE_MINUS_DST_COLOR,
	CFR_BLENDFACTOR_SRC_ALPHA,
	CFR_BLENDFACTOR_ONE_MINUS_SRC_ALPHA,
	CFR_BLENDFACTOR_DST_ALPHA,
	CFR_BLENDFACTOR_ONE_MINUS_DST_ALPHA
} CFR_BlendFactor;

typedef enum CFR_BlendOp {
	CFR_BLEND_OP_ADD,
	CFR_BLEND_OP_SUBTRACT,
	CFR_BLEND_OP_REVERSE_SUBTRACT,
	CFR_BLEND_OP_MIN,
	CFR_BLEND_OP_MAX
} CFR_BlendOp;

typedef struct CFR_BlendState {
	bool enabled;
	CFR_BlendFactor rgb_src_blend_factor;
	CFR_BlendFactor rgb_dst_blend_factor;
	CFR_BlendOp rgb_op;
	CFR_BlendFactor alpha_src_blend_factor;
	CFR_BlendFactor alpha_dst_blend_factor;
	CFR_BlendOp alpha_op;
} CFR_BlendState;

typedef struct CFR_RenderState {
	CFR_BlendState blend;
	bool depth_test_enabled;
	bool depth_write_enabled;
	bool stencil_enabled;
} CFR_RenderState;

//--------------------------------------------------------------------------------------------------
// Vertex and mesh types
//--------------------------------------------------------------------------------------------------

typedef enum CFR_VertexFormat {
	CFR_VERTEX_FORMAT_FLOAT,
	CFR_VERTEX_FORMAT_FLOAT2,
	CFR_VERTEX_FORMAT_FLOAT3,
	CFR_VERTEX_FORMAT_FLOAT4,
	CFR_VERTEX_FORMAT_UBYTE4,
	CFR_VERTEX_FORMAT_UBYTE4_NORM,
	CFR_VERTEX_FORMAT_INT,
	CFR_VERTEX_FORMAT_UINT
} CFR_VertexFormat;

typedef struct CFR_VertexAttribute {
	const char* name;
	CFR_VertexFormat format;
	int offset;
} CFR_VertexAttribute;

//--------------------------------------------------------------------------------------------------
// Shader uniforms
//--------------------------------------------------------------------------------------------------

typedef enum CFR_UniformType {
	CFR_UNIFORM_TYPE_FLOAT,
	CFR_UNIFORM_TYPE_FLOAT2,
	CFR_UNIFORM_TYPE_FLOAT3,
	CFR_UNIFORM_TYPE_FLOAT4,
	CFR_UNIFORM_TYPE_INT,
	CFR_UNIFORM_TYPE_INT2,
	CFR_UNIFORM_TYPE_INT4,
	CFR_UNIFORM_TYPE_MAT4
} CFR_UniformType;

//--------------------------------------------------------------------------------------------------
// Renderer lifecycle
//--------------------------------------------------------------------------------------------------

/**
 * Create a new renderer instance
 * @param device SDL GPU device to use for rendering
 * @param window SDL window to render to
 * @return Renderer instance or NULL on failure
 */
CFR_Renderer* cfr_create_renderer(SDL_GPUDevice* device, SDL_Window* window);

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

//--------------------------------------------------------------------------------------------------
// Texture management
//--------------------------------------------------------------------------------------------------

/**
 * Create a texture
 * @param renderer Renderer instance
 * @param params Texture parameters
 * @return Texture handle
 */
CFR_Texture cfr_make_texture(CFR_Renderer* renderer, CFR_TextureParams params);

/**
 * Update texture data
 * @param renderer Renderer instance
 * @param texture Texture to update
 * @param pixels Pixel data
 * @param size Size of pixel data in bytes
 */
void cfr_texture_update(CFR_Renderer* renderer, CFR_Texture texture, void* pixels, int size);

/**
 * Destroy a texture
 * @param renderer Renderer instance
 * @param texture Texture to destroy
 */
void cfr_destroy_texture(CFR_Renderer* renderer, CFR_Texture texture);

//--------------------------------------------------------------------------------------------------
// Canvas (render target) management
//--------------------------------------------------------------------------------------------------

/**
 * Create a canvas (render target)
 * @param renderer Renderer instance
 * @param width Canvas width
 * @param height Canvas height
 * @return Canvas handle
 */
CFR_Canvas cfr_make_canvas(CFR_Renderer* renderer, int width, int height);

/**
 * Get the texture associated with a canvas
 * @param renderer Renderer instance
 * @param canvas Canvas handle
 * @return Texture handle
 */
CFR_Texture cfr_canvas_get_texture(CFR_Renderer* renderer, CFR_Canvas canvas);

/**
 * Destroy a canvas
 * @param renderer Renderer instance
 * @param canvas Canvas to destroy
 */
void cfr_destroy_canvas(CFR_Renderer* renderer, CFR_Canvas canvas);

/**
 * Set the active canvas for rendering
 * @param renderer Renderer instance
 * @param canvas Canvas to render to (use {0} for screen)
 * @param clear Whether to clear the canvas
 */
void cfr_apply_canvas(CFR_Renderer* renderer, CFR_Canvas canvas, bool clear);

/**
 * Clear the current canvas
 * @param renderer Renderer instance
 * @param color Clear color
 */
void cfr_clear_canvas(CFR_Renderer* renderer, CFR_Color color);

//--------------------------------------------------------------------------------------------------
// Mesh management
//--------------------------------------------------------------------------------------------------

/**
 * Create a mesh
 * @param renderer Renderer instance
 * @param vertex_buffer_size Size of vertex buffer in bytes
 * @param attributes Vertex attributes
 * @param attribute_count Number of attributes
 * @param vertex_stride Size of each vertex in bytes
 * @return Mesh handle
 */
CFR_Mesh cfr_make_mesh(CFR_Renderer* renderer, int vertex_buffer_size, 
                       CFR_VertexAttribute* attributes, int attribute_count, int vertex_stride);

/**
 * Update mesh vertex data
 * @param renderer Renderer instance
 * @param mesh Mesh to update
 * @param vertices Vertex data
 * @param vertex_count Number of vertices
 */
void cfr_mesh_update_vertex_data(CFR_Renderer* renderer, CFR_Mesh mesh, void* vertices, int vertex_count);

/**
 * Destroy a mesh
 * @param renderer Renderer instance
 * @param mesh Mesh to destroy
 */
void cfr_destroy_mesh(CFR_Renderer* renderer, CFR_Mesh mesh);

/**
 * Set the active mesh for rendering
 * @param renderer Renderer instance
 * @param mesh Mesh to use
 */
void cfr_apply_mesh(CFR_Renderer* renderer, CFR_Mesh mesh);

//--------------------------------------------------------------------------------------------------
// Shader management
//--------------------------------------------------------------------------------------------------

/**
 * Create a shader from bytecode
 * @param renderer Renderer instance
 * @param vs_bytecode Vertex shader bytecode
 * @param vs_size Vertex shader bytecode size
 * @param fs_bytecode Fragment shader bytecode
 * @param fs_size Fragment shader bytecode size
 * @return Shader handle
 */
CFR_Shader cfr_make_shader(CFR_Renderer* renderer, 
                           const void* vs_bytecode, size_t vs_size,
                           const void* fs_bytecode, size_t fs_size);

/**
 * Destroy a shader
 * @param renderer Renderer instance
 * @param shader Shader to destroy
 */
void cfr_destroy_shader(CFR_Renderer* renderer, CFR_Shader shader);

//--------------------------------------------------------------------------------------------------
// Material management
//--------------------------------------------------------------------------------------------------

/**
 * Create a material
 * @param renderer Renderer instance
 * @return Material handle
 */
CFR_Material cfr_make_material(CFR_Renderer* renderer);

/**
 * Set a texture on a material
 * @param renderer Renderer instance
 * @param material Material to update
 * @param name Uniform name
 * @param texture Texture to set
 */
void cfr_material_set_texture(CFR_Renderer* renderer, CFR_Material material, 
                               const char* name, CFR_Texture texture);

/**
 * Set a uniform on a material
 * @param renderer Renderer instance
 * @param material Material to update
 * @param name Uniform name
 * @param data Uniform data
 * @param type Uniform type
 * @param array_length Array length (1 for single values)
 */
void cfr_material_set_uniform(CFR_Renderer* renderer, CFR_Material material,
                               const char* name, void* data, CFR_UniformType type, int array_length);

/**
 * Set render state on a material
 * @param renderer Renderer instance
 * @param material Material to update
 * @param render_state Render state to set
 */
void cfr_material_set_render_state(CFR_Renderer* renderer, CFR_Material material, 
                                    CFR_RenderState render_state);

/**
 * Destroy a material
 * @param renderer Renderer instance
 * @param material Material to destroy
 */
void cfr_destroy_material(CFR_Renderer* renderer, CFR_Material material);

//--------------------------------------------------------------------------------------------------
// Drawing
//--------------------------------------------------------------------------------------------------

/**
 * Apply a shader and material for drawing
 * @param renderer Renderer instance
 * @param shader Shader to use
 * @param material Material to use
 */
void cfr_apply_shader(CFR_Renderer* renderer, CFR_Shader shader, CFR_Material material);

/**
 * Set viewport for rendering
 * @param renderer Renderer instance
 * @param x Viewport x
 * @param y Viewport y
 * @param w Viewport width
 * @param h Viewport height
 */
void cfr_apply_viewport(CFR_Renderer* renderer, float x, float y, float w, float h);

/**
 * Set scissor rectangle
 * @param renderer Renderer instance
 * @param x Scissor x
 * @param y Scissor y
 * @param w Scissor width
 * @param h Scissor height
 */
void cfr_apply_scissor(CFR_Renderer* renderer, float x, float y, float w, float h);

/**
 * Draw elements using the current mesh, shader, and material
 * @param renderer Renderer instance
 */
void cfr_draw_elements(CFR_Renderer* renderer);

//--------------------------------------------------------------------------------------------------
// Utility functions
//--------------------------------------------------------------------------------------------------

/**
 * Get default texture parameters
 * @param width Texture width
 * @param height Texture height
 * @return Default texture parameters
 */
CFR_TextureParams cfr_texture_defaults(int width, int height);

/**
 * Get default render state
 * @return Default render state
 */
CFR_RenderState cfr_render_state_defaults(void);

/**
 * Create a color from RGBA values (0-255)
 * @param r Red component
 * @param g Green component
 * @param b Blue component
 * @param a Alpha component
 * @return Color
 */
CFR_Color cfr_make_color(uint8_t r, uint8_t g, uint8_t b, uint8_t a);

/**
 * Create a color from RGBA float values (0.0-1.0)
 * @param r Red component
 * @param g Green component
 * @param b Blue component
 * @param a Alpha component
 * @return Color
 */
CFR_Color cfr_make_color_f(float r, float g, float b, float a);

/**
 * Create a vector
 * @param x X component
 * @param y Y component
 * @return Vector
 */
CFR_Vec2 cfr_v2(float x, float y);

#ifdef __cplusplus
}
#endif

#endif // CF_RENDERER_H
