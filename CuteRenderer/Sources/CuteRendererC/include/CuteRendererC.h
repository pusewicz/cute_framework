/*
 * CuteRenderer C API Bridge
 *
 * This header provides the C API bridge for CuteRenderer.
 * It wraps SDL3 GPU functionality for use with the Swift renderer.
 */

#ifndef CUTE_RENDERER_C_H
#define CUTE_RENDERER_C_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// MARK: - Basic Types

/// A 2D vector.
typedef struct CR_V2 {
    float x;
    float y;
} CR_V2;

/// A color with float components (0-1).
typedef struct CR_Color {
    float r;
    float g;
    float b;
    float a;
} CR_Color;

/// A color with byte components (0-255).
typedef struct CR_Pixel {
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t a;
} CR_Pixel;

/// An axis-aligned bounding box.
typedef struct CR_AABB {
    CR_V2 min;
    CR_V2 max;
} CR_AABB;

/// A circle shape.
typedef struct CR_Circle {
    CR_V2 position;
    float radius;
} CR_Circle;

/// A capsule shape.
typedef struct CR_Capsule {
    CR_V2 a;
    CR_V2 b;
    float radius;
} CR_Capsule;

/// An integer rectangle.
typedef struct CR_Rect {
    int x;
    int y;
    int width;
    int height;
} CR_Rect;

// MARK: - Opaque Handle Types

/// Texture handle.
typedef struct CR_Texture { uint64_t id; } CR_Texture;

/// Canvas (render target) handle.
typedef struct CR_Canvas { uint64_t id; } CR_Canvas;

/// Shader handle.
typedef struct CR_Shader { uint64_t id; } CR_Shader;

/// Material handle.
typedef struct CR_Material { uint64_t id; } CR_Material;

/// Mesh handle.
typedef struct CR_Mesh { uint64_t id; } CR_Mesh;

// MARK: - Enums

/// Pixel format enumeration.
typedef enum CR_PixelFormat {
    CR_PIXEL_FORMAT_INVALID = -1,
    CR_PIXEL_FORMAT_A8_UNORM = 0,
    CR_PIXEL_FORMAT_R8_UNORM = 1,
    CR_PIXEL_FORMAT_R8G8_UNORM = 2,
    CR_PIXEL_FORMAT_R8G8B8A8_UNORM = 3,
    CR_PIXEL_FORMAT_R16_UNORM = 4,
    CR_PIXEL_FORMAT_R16G16_UNORM = 5,
    CR_PIXEL_FORMAT_R16G16B16A16_UNORM = 6,
    CR_PIXEL_FORMAT_R32G32B32A32_FLOAT = 31,
    CR_PIXEL_FORMAT_D16_UNORM = 51,
    CR_PIXEL_FORMAT_D24_UNORM = 52,
    CR_PIXEL_FORMAT_D32_FLOAT = 53,
} CR_PixelFormat;

/// Texture filter mode.
typedef enum CR_Filter {
    CR_FILTER_NEAREST = 0,
    CR_FILTER_LINEAR = 1,
} CR_Filter;

/// Texture wrap mode.
typedef enum CR_WrapMode {
    CR_WRAP_MODE_REPEAT = 0,
    CR_WRAP_MODE_CLAMP_TO_EDGE = 1,
    CR_WRAP_MODE_MIRRORED_REPEAT = 2,
} CR_WrapMode;

/// Blend factor.
typedef enum CR_BlendFactor {
    CR_BLEND_FACTOR_ZERO = 0,
    CR_BLEND_FACTOR_ONE = 1,
    CR_BLEND_FACTOR_SRC_COLOR = 2,
    CR_BLEND_FACTOR_ONE_MINUS_SRC_COLOR = 3,
    CR_BLEND_FACTOR_DST_COLOR = 4,
    CR_BLEND_FACTOR_ONE_MINUS_DST_COLOR = 5,
    CR_BLEND_FACTOR_SRC_ALPHA = 6,
    CR_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA = 7,
    CR_BLEND_FACTOR_DST_ALPHA = 8,
    CR_BLEND_FACTOR_ONE_MINUS_DST_ALPHA = 9,
} CR_BlendFactor;

/// Blend operation.
typedef enum CR_BlendOp {
    CR_BLEND_OP_ADD = 0,
    CR_BLEND_OP_SUBTRACT = 1,
    CR_BLEND_OP_REVERSE_SUBTRACT = 2,
    CR_BLEND_OP_MIN = 3,
    CR_BLEND_OP_MAX = 4,
} CR_BlendOp;

/// Backend type.
typedef enum CR_BackendType {
    CR_BACKEND_INVALID = -1,
    CR_BACKEND_VULKAN = 0,
    CR_BACKEND_D3D11 = 1,
    CR_BACKEND_D3D12 = 2,
    CR_BACKEND_METAL = 3,
    CR_BACKEND_GLES3 = 5,
} CR_BackendType;

// MARK: - Texture Parameters

/// Texture usage flags.
typedef uint32_t CR_TextureUsageFlags;

#define CR_TEXTURE_USAGE_SAMPLER                0x00000001
#define CR_TEXTURE_USAGE_COLOR_TARGET           0x00000002
#define CR_TEXTURE_USAGE_DEPTH_STENCIL_TARGET   0x00000004
#define CR_TEXTURE_USAGE_GRAPHICS_STORAGE_READ  0x00000008
#define CR_TEXTURE_USAGE_COMPUTE_STORAGE_READ   0x00000020
#define CR_TEXTURE_USAGE_COMPUTE_STORAGE_WRITE  0x00000040

/// Texture creation parameters.
typedef struct CR_TextureParams {
    CR_PixelFormat pixel_format;
    CR_TextureUsageFlags usage;
    CR_Filter filter;
    CR_WrapMode wrap_u;
    CR_WrapMode wrap_v;
    int width;
    int height;
    int mip_count;
    bool generate_mipmaps;
    float mip_lod_bias;
    float max_anisotropy;
    bool stream;
} CR_TextureParams;

// MARK: - Initialization

/// Initializes the renderer with the given window handle.
/// Returns true on success.
bool cr_init(void* window_handle);

/// Shuts down the renderer.
void cr_shutdown(void);

/// Queries the current backend type.
CR_BackendType cr_query_backend(void);

// MARK: - Texture Functions

/// Creates a texture with the given parameters.
CR_Texture cr_make_texture(CR_TextureParams params);

/// Destroys a texture.
void cr_destroy_texture(CR_Texture texture);

/// Updates texture data.
void cr_texture_update(CR_Texture texture, const void* data, size_t size);

/// Updates a specific mip level.
void cr_texture_update_mip(CR_Texture texture, const void* data, size_t size, int mip_level);

/// Generates mipmaps for a texture.
void cr_generate_mipmaps(CR_Texture texture);

// MARK: - Canvas Functions

/// Canvas creation parameters.
typedef struct CR_CanvasParams {
    int width;
    int height;
    CR_PixelFormat color_format;
    CR_PixelFormat depth_format;
    int sample_count;
} CR_CanvasParams;

/// Creates a canvas (render target).
CR_Canvas cr_make_canvas(CR_CanvasParams params);

/// Destroys a canvas.
void cr_destroy_canvas(CR_Canvas canvas);

/// Sets the current canvas for rendering.
void cr_apply_canvas(CR_Canvas canvas);

/// Clears the current canvas with a color.
void cr_clear_color(CR_Color color);

/// Clears with color and depth.
void cr_clear_color_depth(CR_Color color, float depth);

// MARK: - Shader Functions

/// Sets the shader directory for includes.
void cr_shader_directory(const char* path);

/// Shader change callback type.
typedef void (*CR_ShaderChangedFn)(const char* path, void* udata);

/// Registers a callback for shader file changes.
void cr_shader_on_changed(CR_ShaderChangedFn callback, void* udata);

/// Creates a shader from source files.
CR_Shader cr_make_shader(const char* vertex_path, const char* fragment_path);

/// Creates a shader from source strings.
CR_Shader cr_make_shader_from_source(const char* vertex_src, const char* fragment_src);

/// Destroys a shader.
void cr_destroy_shader(CR_Shader shader);

// MARK: - Material Functions

/// Creates a material.
CR_Material cr_make_material(void);

/// Destroys a material.
void cr_destroy_material(CR_Material material);

/// Sets a float uniform.
void cr_material_set_uniform_float(CR_Material material, const char* name, float value);

/// Sets a vec2 uniform.
void cr_material_set_uniform_v2(CR_Material material, const char* name, CR_V2 value);

/// Sets a color uniform.
void cr_material_set_uniform_color(CR_Material material, const char* name, CR_Color value);

/// Sets a texture binding.
void cr_material_set_texture(CR_Material material, const char* name, CR_Texture texture);

// MARK: - Mesh Functions

/// Vertex attribute description.
typedef struct CR_VertexAttribute {
    const char* name;
    int format;
    int offset;
} CR_VertexAttribute;

/// Creates a mesh with the given vertex layout.
CR_Mesh cr_make_mesh(const CR_VertexAttribute* attributes, int attribute_count, int stride);

/// Destroys a mesh.
void cr_destroy_mesh(CR_Mesh mesh);

/// Updates mesh vertex data.
void cr_mesh_update_vertices(CR_Mesh mesh, const void* data, size_t size, int count);

/// Updates mesh index data.
void cr_mesh_update_indices(CR_Mesh mesh, const void* data, size_t size, int count, bool use_32bit);

// MARK: - Rendering

/// Applies a mesh for rendering.
void cr_apply_mesh(CR_Mesh mesh);

/// Applies a shader and material for rendering.
void cr_apply_shader(CR_Shader shader, CR_Material material);

/// Draws indexed elements.
void cr_draw_elements(int count, int offset);

/// Draws non-indexed arrays.
void cr_draw_arrays(int count, int offset);

// MARK: - Draw API

/// Pushes a draw layer.
void cr_draw_push_layer(int layer);

/// Pops a draw layer.
int cr_draw_pop_layer(void);

/// Pushes a draw color.
void cr_draw_push_color(CR_Color color);

/// Pops a draw color.
CR_Color cr_draw_pop_color(void);

/// Pushes antialias state.
void cr_draw_push_antialias(bool enabled);

/// Pops antialias state.
bool cr_draw_pop_antialias(void);

/// Draws a quad wireframe.
void cr_draw_quad(CR_AABB aabb, float thickness, float chubbiness);

/// Draws a filled quad.
void cr_draw_quad_fill(CR_AABB aabb, float chubbiness);

/// Draws a circle wireframe.
void cr_draw_circle(CR_V2 center, float radius, float thickness);

/// Draws a filled circle.
void cr_draw_circle_fill(CR_V2 center, float radius);

/// Draws a capsule wireframe.
void cr_draw_capsule(CR_V2 a, CR_V2 b, float radius, float thickness);

/// Draws a filled capsule.
void cr_draw_capsule_fill(CR_V2 a, CR_V2 b, float radius);

/// Draws a triangle wireframe.
void cr_draw_tri(CR_V2 p0, CR_V2 p1, CR_V2 p2, float thickness, float chubbiness);

/// Draws a filled triangle.
void cr_draw_tri_fill(CR_V2 p0, CR_V2 p1, CR_V2 p2, float chubbiness);

/// Draws a line.
void cr_draw_line(CR_V2 from, CR_V2 to, float thickness);

/// Draws a polyline.
void cr_draw_polyline(const CR_V2* points, int count, float thickness, bool loop);

/// Flushes pending draw commands.
void cr_draw_flush(void);

#ifdef __cplusplus
}
#endif

#endif /* CUTE_RENDERER_C_H */
