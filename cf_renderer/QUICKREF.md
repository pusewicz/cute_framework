# CF Renderer - Quick Reference

## Essential Info

**Location:** `cf_renderer/` directory  
**Language:** C99  
**Dependencies:** SDL3 only  
**License:** Dual zlib/Unlicense  

## API at a Glance

### Setup & Cleanup
```c
CFR_Renderer* cfr_create_renderer(SDL_GPUDevice* device, SDL_Window* window);
void cfr_destroy_renderer(CFR_Renderer* renderer);
```

### Frame Control
```c
void cfr_begin_frame(CFR_Renderer* renderer);
void cfr_end_frame(CFR_Renderer* renderer);
```

### Textures
```c
CFR_Texture cfr_make_texture(CFR_Renderer* r, CFR_TextureParams params);
void cfr_texture_update(CFR_Renderer* r, CFR_Texture tex, void* pixels, int size);
void cfr_destroy_texture(CFR_Renderer* r, CFR_Texture tex);
```

### Canvases (Render Targets)
```c
CFR_Canvas cfr_make_canvas(CFR_Renderer* r, int w, int h);
void cfr_apply_canvas(CFR_Renderer* r, CFR_Canvas canvas, bool clear);
void cfr_destroy_canvas(CFR_Renderer* r, CFR_Canvas canvas);
```

### Meshes
```c
CFR_Mesh cfr_make_mesh(CFR_Renderer* r, int buffer_size, 
                       CFR_VertexAttribute* attrs, int count, int stride);
void cfr_mesh_update_vertex_data(CFR_Renderer* r, CFR_Mesh m, void* verts, int count);
void cfr_apply_mesh(CFR_Renderer* r, CFR_Mesh mesh);
void cfr_destroy_mesh(CFR_Renderer* r, CFR_Mesh mesh);
```

### Shaders
```c
CFR_Shader cfr_make_shader(CFR_Renderer* r,
                           const void* vs_spirv, size_t vs_size,
                           const void* fs_spirv, size_t fs_size);
void cfr_destroy_shader(CFR_Renderer* r, CFR_Shader shader);
```

### Materials
```c
CFR_Material cfr_make_material(CFR_Renderer* r);
void cfr_material_set_texture(CFR_Renderer* r, CFR_Material m, const char* name, CFR_Texture tex);
void cfr_material_set_uniform(CFR_Renderer* r, CFR_Material m, const char* name, 
                               void* data, CFR_UniformType type, int array_len);
void cfr_material_set_render_state(CFR_Renderer* r, CFR_Material m, CFR_RenderState state);
void cfr_destroy_material(CFR_Renderer* r, CFR_Material mat);
```

### Drawing
```c
void cfr_apply_shader(CFR_Renderer* r, CFR_Shader shader, CFR_Material mat);
void cfr_apply_viewport(CFR_Renderer* r, float x, float y, float w, float h);
void cfr_apply_scissor(CFR_Renderer* r, float x, float y, float w, float h);
void cfr_draw_elements(CFR_Renderer* r);
```

### Helpers
```c
CFR_TextureParams cfr_texture_defaults(int w, int h);
CFR_RenderState cfr_render_state_defaults(void);
CFR_Color cfr_make_color(uint8_t r, uint8_t g, uint8_t b, uint8_t a);
CFR_Color cfr_make_color_f(float r, float g, float b, float a);
CFR_Vec2 cfr_v2(float x, float y);
```

## Minimal Example

```c
#include "cf_renderer.h"
#include <SDL3/SDL.h>

int main(void) {
    SDL_Init(SDL_INIT_VIDEO);
    SDL_Window* window = SDL_CreateWindow("Game", 640, 480, SDL_WINDOW_VULKAN);
    SDL_GPUDevice* device = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV, true, NULL);
    SDL_ClaimWindowForGPUDevice(device, window);
    
    CFR_Renderer* renderer = cfr_create_renderer(device, window);
    
    // Create resources (mesh, shader, material)
    // ...
    
    while (running) {
        cfr_begin_frame(renderer);
        cfr_apply_canvas(renderer, (CFR_Canvas){0}, true);
        cfr_apply_mesh(renderer, mesh);
        cfr_apply_shader(renderer, shader, material);
        cfr_draw_elements(renderer);
        cfr_end_frame(renderer);
    }
    
    cfr_destroy_renderer(renderer);
    SDL_DestroyGPUDevice(device);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
```

## Key Types

```c
typedef struct { uint64_t id; } CFR_Texture;
typedef struct { uint64_t id; } CFR_Canvas;
typedef struct { uint64_t id; } CFR_Mesh;
typedef struct { uint64_t id; } CFR_Shader;
typedef struct { uint64_t id; } CFR_Material;

typedef struct { float x, y; } CFR_Vec2;
typedef struct { float r, g, b, a; } CFR_Color;
typedef struct { uint8_t r, g, b, a; } CFR_Pixel;
```

## Vertex Formats

```c
CFR_VERTEX_FORMAT_FLOAT       // 1 float
CFR_VERTEX_FORMAT_FLOAT2      // 2 floats (vec2)
CFR_VERTEX_FORMAT_FLOAT3      // 3 floats (vec3)
CFR_VERTEX_FORMAT_FLOAT4      // 4 floats (vec4)
CFR_VERTEX_FORMAT_UBYTE4      // 4 unsigned bytes
CFR_VERTEX_FORMAT_UBYTE4_NORM // 4 normalized unsigned bytes
CFR_VERTEX_FORMAT_INT         // 1 int
CFR_VERTEX_FORMAT_UINT        // 1 unsigned int
```

## Uniform Types

```c
CFR_UNIFORM_TYPE_FLOAT    // 1 float
CFR_UNIFORM_TYPE_FLOAT2   // vec2
CFR_UNIFORM_TYPE_FLOAT3   // vec3
CFR_UNIFORM_TYPE_FLOAT4   // vec4
CFR_UNIFORM_TYPE_INT      // 1 int
CFR_UNIFORM_TYPE_INT2     // ivec2
CFR_UNIFORM_TYPE_INT4     // ivec4
CFR_UNIFORM_TYPE_MAT4     // mat4
```

## Blend Factors

```c
CFR_BLENDFACTOR_ZERO
CFR_BLENDFACTOR_ONE
CFR_BLENDFACTOR_SRC_COLOR
CFR_BLENDFACTOR_ONE_MINUS_SRC_COLOR
CFR_BLENDFACTOR_DST_COLOR
CFR_BLENDFACTOR_ONE_MINUS_DST_COLOR
CFR_BLENDFACTOR_SRC_ALPHA
CFR_BLENDFACTOR_ONE_MINUS_SRC_ALPHA
CFR_BLENDFACTOR_DST_ALPHA
CFR_BLENDFACTOR_ONE_MINUS_DST_ALPHA
```

## Blend Operations

```c
CFR_BLEND_OP_ADD
CFR_BLEND_OP_SUBTRACT
CFR_BLEND_OP_REVERSE_SUBTRACT
CFR_BLEND_OP_MIN
CFR_BLEND_OP_MAX
```

## Common Patterns

**Render to texture:**
```c
CFR_Canvas canvas = cfr_make_canvas(renderer, 512, 512);
cfr_apply_canvas(renderer, canvas, true);
// ... draw to canvas ...
CFR_Texture tex = cfr_canvas_get_texture(renderer, canvas);
```

**Alpha blending:**
```c
CFR_RenderState state = cfr_render_state_defaults();
state.blend.enabled = true;
state.blend.rgb_src_blend_factor = CFR_BLENDFACTOR_SRC_ALPHA;
state.blend.rgb_dst_blend_factor = CFR_BLENDFACTOR_ONE_MINUS_SRC_ALPHA;
cfr_material_set_render_state(renderer, material, state);
```

**Additive blending:**
```c
state.blend.rgb_dst_blend_factor = CFR_BLENDFACTOR_ONE;
```

## Shader Compilation

```bash
# Write GLSL
cat > shader.vert << EOF
#version 450
layout(location = 0) in vec2 position;
void main() {
    gl_Position = vec4(position, 0.0, 1.0);
}
EOF

# Compile to SPIR-V
glslangValidator -V shader.vert -o shader.vert.spv

# Load in C
unsigned char* load_file(const char* path, size_t* size);
size_t vs_size;
unsigned char* vs = load_file("shader.vert.spv", &vs_size);
CFR_Shader shader = cfr_make_shader(renderer, vs, vs_size, fs, fs_size);
```

## Build

```bash
mkdir build && cd build
cmake ..
cmake --build .
```

## CMake Integration

```cmake
add_subdirectory(path/to/cf_renderer)
target_link_libraries(my_game PRIVATE cf_renderer)
```

## Documentation Files

- `README.md` - Overview & features
- `INTEGRATION.md` - Complete guide with examples
- `EXTRACTION.md` - Technical architecture
- `SUMMARY.md` - High-level summary
- `examples/simple_triangle.c` - Working example

## Quick Tips

1. **Always call `cfr_begin_frame()` before rendering**
2. **Call `cfr_end_frame()` to present**
3. **Canvas ID 0 means render to screen**
4. **Shaders must be SPIR-V bytecode**
5. **Vertex attributes must match shader inputs**
6. **Call `cfr_apply_canvas()` before drawing**
7. **Call `cfr_apply_mesh()` before `cfr_apply_shader()`**

## Common Issues

**Black screen?**
- Check viewport is set
- Verify shaders are valid SPIR-V
- Ensure blend state allows rendering

**Crash on draw?**
- Verify mesh has data
- Check canvas is applied
- Ensure shader/material are valid

**Can't create device?**
- Install GPU drivers
- Try different shader format
- Check window flags match backend

## More Info

See full documentation in the `cf_renderer/` directory:
- Complete API: `include/cf_renderer.h`
- Integration guide: `INTEGRATION.md`
- Examples: `examples/simple_triangle.c`
