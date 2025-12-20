/*
 * CuteRenderer C Implementation
 *
 * This is a stub implementation. The actual implementation would wrap
 * SDL3 GPU functionality.
 */

#include "include/CuteRendererC.h"
#include <stdlib.h>

// Stub implementations - in a real project these would call SDL3 GPU functions

bool cr_init(void* window_handle) {
    (void)window_handle;
    return true;
}

void cr_shutdown(void) {
    // Cleanup resources
}

CR_BackendType cr_query_backend(void) {
    return CR_BACKEND_VULKAN;
}

// Texture functions
CR_Texture cr_make_texture(CR_TextureParams params) {
    (void)params;
    CR_Texture tex = { .id = (uint64_t)rand() };
    return tex;
}

void cr_destroy_texture(CR_Texture texture) {
    (void)texture;
}

void cr_texture_update(CR_Texture texture, const void* data, size_t size) {
    (void)texture;
    (void)data;
    (void)size;
}

void cr_texture_update_mip(CR_Texture texture, const void* data, size_t size, int mip_level) {
    (void)texture;
    (void)data;
    (void)size;
    (void)mip_level;
}

void cr_generate_mipmaps(CR_Texture texture) {
    (void)texture;
}

// Canvas functions
CR_Canvas cr_make_canvas(CR_CanvasParams params) {
    (void)params;
    CR_Canvas canvas = { .id = (uint64_t)rand() };
    return canvas;
}

void cr_destroy_canvas(CR_Canvas canvas) {
    (void)canvas;
}

void cr_apply_canvas(CR_Canvas canvas) {
    (void)canvas;
}

void cr_clear_color(CR_Color color) {
    (void)color;
}

void cr_clear_color_depth(CR_Color color, float depth) {
    (void)color;
    (void)depth;
}

// Shader functions
void cr_shader_directory(const char* path) {
    (void)path;
}

void cr_shader_on_changed(CR_ShaderChangedFn callback, void* udata) {
    (void)callback;
    (void)udata;
}

CR_Shader cr_make_shader(const char* vertex_path, const char* fragment_path) {
    (void)vertex_path;
    (void)fragment_path;
    CR_Shader shader = { .id = (uint64_t)rand() };
    return shader;
}

CR_Shader cr_make_shader_from_source(const char* vertex_src, const char* fragment_src) {
    (void)vertex_src;
    (void)fragment_src;
    CR_Shader shader = { .id = (uint64_t)rand() };
    return shader;
}

void cr_destroy_shader(CR_Shader shader) {
    (void)shader;
}

// Material functions
CR_Material cr_make_material(void) {
    CR_Material material = { .id = (uint64_t)rand() };
    return material;
}

void cr_destroy_material(CR_Material material) {
    (void)material;
}

void cr_material_set_uniform_float(CR_Material material, const char* name, float value) {
    (void)material;
    (void)name;
    (void)value;
}

void cr_material_set_uniform_v2(CR_Material material, const char* name, CR_V2 value) {
    (void)material;
    (void)name;
    (void)value;
}

void cr_material_set_uniform_color(CR_Material material, const char* name, CR_Color value) {
    (void)material;
    (void)name;
    (void)value;
}

void cr_material_set_texture(CR_Material material, const char* name, CR_Texture texture) {
    (void)material;
    (void)name;
    (void)texture;
}

// Mesh functions
CR_Mesh cr_make_mesh(const CR_VertexAttribute* attributes, int attribute_count, int stride) {
    (void)attributes;
    (void)attribute_count;
    (void)stride;
    CR_Mesh mesh = { .id = (uint64_t)rand() };
    return mesh;
}

void cr_destroy_mesh(CR_Mesh mesh) {
    (void)mesh;
}

void cr_mesh_update_vertices(CR_Mesh mesh, const void* data, size_t size, int count) {
    (void)mesh;
    (void)data;
    (void)size;
    (void)count;
}

void cr_mesh_update_indices(CR_Mesh mesh, const void* data, size_t size, int count, bool use_32bit) {
    (void)mesh;
    (void)data;
    (void)size;
    (void)count;
    (void)use_32bit;
}

void cr_apply_mesh(CR_Mesh mesh) {
    (void)mesh;
}

void cr_apply_shader(CR_Shader shader, CR_Material material) {
    (void)shader;
    (void)material;
}

void cr_draw_elements(int count, int offset) {
    (void)count;
    (void)offset;
}

void cr_draw_arrays(int count, int offset) {
    (void)count;
    (void)offset;
}

// Draw state
static int s_layer = 0;
static CR_Color s_color = { 1.0f, 1.0f, 1.0f, 1.0f };
static bool s_antialias = true;

void cr_draw_push_layer(int layer) {
    s_layer = layer;
}

int cr_draw_pop_layer(void) {
    int layer = s_layer;
    s_layer = 0;
    return layer;
}

void cr_draw_push_color(CR_Color color) {
    s_color = color;
}

CR_Color cr_draw_pop_color(void) {
    CR_Color color = s_color;
    s_color = (CR_Color){ 1.0f, 1.0f, 1.0f, 1.0f };
    return color;
}

void cr_draw_push_antialias(bool enabled) {
    s_antialias = enabled;
}

bool cr_draw_pop_antialias(void) {
    bool aa = s_antialias;
    s_antialias = true;
    return aa;
}

// Draw functions (stubs)
void cr_draw_quad(CR_AABB aabb, float thickness, float chubbiness) {
    (void)aabb;
    (void)thickness;
    (void)chubbiness;
}

void cr_draw_quad_fill(CR_AABB aabb, float chubbiness) {
    (void)aabb;
    (void)chubbiness;
}

void cr_draw_circle(CR_V2 center, float radius, float thickness) {
    (void)center;
    (void)radius;
    (void)thickness;
}

void cr_draw_circle_fill(CR_V2 center, float radius) {
    (void)center;
    (void)radius;
}

void cr_draw_capsule(CR_V2 a, CR_V2 b, float radius, float thickness) {
    (void)a;
    (void)b;
    (void)radius;
    (void)thickness;
}

void cr_draw_capsule_fill(CR_V2 a, CR_V2 b, float radius) {
    (void)a;
    (void)b;
    (void)radius;
}

void cr_draw_tri(CR_V2 p0, CR_V2 p1, CR_V2 p2, float thickness, float chubbiness) {
    (void)p0;
    (void)p1;
    (void)p2;
    (void)thickness;
    (void)chubbiness;
}

void cr_draw_tri_fill(CR_V2 p0, CR_V2 p1, CR_V2 p2, float chubbiness) {
    (void)p0;
    (void)p1;
    (void)p2;
    (void)chubbiness;
}

void cr_draw_line(CR_V2 from, CR_V2 to, float thickness) {
    (void)from;
    (void)to;
    (void)thickness;
}

void cr_draw_polyline(const CR_V2* points, int count, float thickness, bool loop) {
    (void)points;
    (void)count;
    (void)thickness;
    (void)loop;
}

void cr_draw_flush(void) {
    // Flush all pending draw commands
}
