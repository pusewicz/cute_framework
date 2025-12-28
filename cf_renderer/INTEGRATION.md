# CF Renderer Integration Guide

This guide shows how to integrate CF Renderer into your SDL3 project.

## Quick Integration

### Option 1: As a Subdirectory

1. Copy the `cf_renderer/` directory into your project
2. Add to your CMakeLists.txt:

```cmake
add_subdirectory(cf_renderer)
target_link_libraries(your_game PRIVATE cf_renderer)
```

### Option 2: As a Separate Build

1. Build CF Renderer separately:
```bash
cd cf_renderer
mkdir build && cd build
cmake ..
cmake --build .
cmake --install . --prefix /usr/local
```

2. Find and link in your project:
```cmake
find_package(cf_renderer REQUIRED)
target_link_libraries(your_game PRIVATE CFRenderer::cf_renderer)
```

## Complete Example

Here's a complete, minimal SDL3 + CF Renderer application:

```c
#include "cf_renderer.h"
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <stdbool.h>

int main(int argc, char* argv[])
{
    // 1. Initialize SDL
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_Log("SDL_Init failed: %s", SDL_GetError());
        return 1;
    }

    // 2. Create window
    SDL_Window* window = SDL_CreateWindow(
        "CF Renderer Example",
        640, 480,
        SDL_WINDOW_VULKAN  // Can also use SDL_WINDOW_METAL or SDL_WINDOW_HIGH_PIXEL_DENSITY
    );
    
    if (!window) {
        SDL_Log("Window creation failed: %s", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    // 3. Create GPU device
    SDL_GPUDevice* device = SDL_CreateGPUDevice(
        SDL_GPU_SHADERFORMAT_SPIRV,  // SPIR-V for Vulkan/D3D12, can also use others
        true,                         // Enable debug mode
        NULL                          // Default name
    );
    
    if (!device) {
        SDL_Log("GPU device creation failed: %s", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    // 4. Claim window for GPU rendering
    if (!SDL_ClaimWindowForGPUDevice(device, window)) {
        SDL_Log("Failed to claim window: %s", SDL_GetError());
        SDL_DestroyGPUDevice(device);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    // 5. Create CF Renderer
    CFR_Renderer* renderer = cfr_create_renderer(device, window);
    if (!renderer) {
        SDL_Log("Failed to create renderer");
        SDL_DestroyGPUDevice(device);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    // 6. Load or compile your shaders
    // NOTE: You need actual SPIR-V bytecode here
    // See "Shader Compilation" section below
    const unsigned char* vs_spirv = /* your vertex shader SPIR-V */;
    size_t vs_size = /* vertex shader size */;
    const unsigned char* fs_spirv = /* your fragment shader SPIR-V */;
    size_t fs_size = /* fragment shader size */;
    
    CFR_Shader shader = cfr_make_shader(renderer, vs_spirv, vs_size, fs_spirv, fs_size);

    // 7. Create material
    CFR_Material material = cfr_make_material(renderer);
    CFR_RenderState render_state = cfr_render_state_defaults();
    cfr_material_set_render_state(renderer, material, render_state);

    // 8. Create mesh (example: a quad)
    typedef struct Vertex {
        CFR_Vec2 position;
        CFR_Vec2 texcoord;
        CFR_Color color;
    } Vertex;
    
    Vertex vertices[] = {
        { {-0.5f, -0.5f}, {0.0f, 0.0f}, cfr_make_color_f(1,1,1,1) },
        { { 0.5f, -0.5f}, {1.0f, 0.0f}, cfr_make_color_f(1,1,1,1) },
        { { 0.5f,  0.5f}, {1.0f, 1.0f}, cfr_make_color_f(1,1,1,1) },
        { {-0.5f, -0.5f}, {0.0f, 0.0f}, cfr_make_color_f(1,1,1,1) },
        { { 0.5f,  0.5f}, {1.0f, 1.0f}, cfr_make_color_f(1,1,1,1) },
        { {-0.5f,  0.5f}, {0.0f, 1.0f}, cfr_make_color_f(1,1,1,1) },
    };
    
    CFR_VertexAttribute attributes[] = {
        { "position", CFR_VERTEX_FORMAT_FLOAT2, offsetof(Vertex, position) },
        { "texcoord", CFR_VERTEX_FORMAT_FLOAT2, offsetof(Vertex, texcoord) },
        { "color",    CFR_VERTEX_FORMAT_FLOAT4, offsetof(Vertex, color) }
    };
    
    CFR_Mesh mesh = cfr_make_mesh(renderer, sizeof(vertices), attributes, 3, sizeof(Vertex));
    cfr_mesh_update_vertex_data(renderer, mesh, vertices, 6);

    // 9. Main loop
    bool running = true;
    SDL_Event event;
    
    while (running) {
        // Handle events
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                running = false;
            }
            if (event.type == SDL_EVENT_KEY_DOWN && event.key.key == SDLK_ESCAPE) {
                running = false;
            }
        }

        // Begin rendering
        cfr_begin_frame(renderer);
        
        // Render to screen (canvas ID 0)
        CFR_Canvas screen = { 0 };
        cfr_apply_canvas(renderer, screen, true);
        
        // Set viewport
        int w, h;
        SDL_GetWindowSize(window, &w, &h);
        cfr_apply_viewport(renderer, 0, 0, (float)w, (float)h);
        
        // Draw mesh
        cfr_apply_mesh(renderer, mesh);
        cfr_apply_shader(renderer, shader, material);
        cfr_draw_elements(renderer);
        
        // End frame and present
        cfr_end_frame(renderer);
    }

    // 10. Cleanup
    cfr_destroy_mesh(renderer, mesh);
    cfr_destroy_material(renderer, material);
    cfr_destroy_shader(renderer, shader);
    cfr_destroy_renderer(renderer);
    
    SDL_DestroyGPUDevice(device);
    SDL_DestroyWindow(window);
    SDL_Quit();
    
    return 0;
}
```

## Shader Compilation

CF Renderer expects SPIR-V bytecode. Here's how to create it:

### Using glslangValidator

1. Write your shader in GLSL:

**vertex.glsl:**
```glsl
#version 450

layout(location = 0) in vec2 position;
layout(location = 1) in vec2 texcoord;
layout(location = 2) in vec4 color;

layout(location = 0) out vec2 v_texcoord;
layout(location = 1) out vec4 v_color;

void main() {
    gl_Position = vec4(position, 0.0, 1.0);
    v_texcoord = texcoord;
    v_color = color;
}
```

**fragment.glsl:**
```glsl
#version 450

layout(location = 0) in vec2 v_texcoord;
layout(location = 1) in vec4 v_color;

layout(location = 0) out vec4 outColor;

layout(binding = 0) uniform sampler2D texSampler;

void main() {
    outColor = texture(texSampler, v_texcoord) * v_color;
}
```

2. Compile to SPIR-V:
```bash
glslangValidator -V vertex.glsl -o vertex.spv
glslangValidator -V fragment.glsl -o fragment.spv
```

3. Load in your code:
```c
// Helper function to load SPIR-V file
unsigned char* load_spirv(const char* path, size_t* out_size) {
    FILE* f = fopen(path, "rb");
    if (!f) return NULL;
    
    fseek(f, 0, SEEK_END);
    *out_size = ftell(f);
    fseek(f, 0, SEEK_SET);
    
    unsigned char* data = malloc(*out_size);
    fread(data, 1, *out_size, f);
    fclose(f);
    
    return data;
}

// In your code:
size_t vs_size, fs_size;
unsigned char* vs_data = load_spirv("vertex.spv", &vs_size);
unsigned char* fs_data = load_spirv("fragment.spv", &fs_size);

CFR_Shader shader = cfr_make_shader(renderer, vs_data, vs_size, fs_data, fs_size);

free(vs_data);
free(fs_data);
```

## Texture Usage

```c
// Load pixel data (you need to load PNG/JPEG yourself)
// Example assumes you have raw RGBA data
int width = 256, height = 256;
CFR_Pixel* pixels = /* load your image data */;

// Create texture
CFR_TextureParams params = cfr_texture_defaults(width, height);
params.filter = CFR_FILTER_LINEAR;
CFR_Texture texture = cfr_make_texture(renderer, params);

// Upload pixels
cfr_texture_update(renderer, texture, pixels, width * height * sizeof(CFR_Pixel));

// Use in material (before calling cfr_apply_shader)
cfr_material_set_texture(renderer, material, "texSampler", texture);
```

## Render to Texture

```c
// Create canvas (render target)
CFR_Canvas canvas = cfr_make_canvas(renderer, 512, 512);

// Render to it
cfr_begin_frame(renderer);
cfr_apply_canvas(renderer, canvas, true);  // Clear it
cfr_apply_mesh(renderer, mesh);
cfr_apply_shader(renderer, shader, material);
cfr_draw_elements(renderer);

// Now render canvas to screen
CFR_Canvas screen = { 0 };
cfr_apply_canvas(renderer, screen, true);

// Get canvas as texture
CFR_Texture canvas_tex = cfr_canvas_get_texture(renderer, canvas);
cfr_material_set_texture(renderer, blit_material, "texSampler", canvas_tex);

// Draw full-screen quad with canvas texture
cfr_apply_mesh(renderer, quad_mesh);
cfr_apply_shader(renderer, blit_shader, blit_material);
cfr_draw_elements(renderer);

cfr_end_frame(renderer);
```

## Common Patterns

### Multiple Draw Calls

```c
cfr_begin_frame(renderer);
cfr_apply_canvas(renderer, screen, true);

// First object
cfr_apply_mesh(renderer, mesh1);
cfr_apply_shader(renderer, shader1, material1);
cfr_draw_elements(renderer);

// Second object (different mesh/shader/material)
cfr_apply_mesh(renderer, mesh2);
cfr_apply_shader(renderer, shader2, material2);
cfr_draw_elements(renderer);

cfr_end_frame(renderer);
```

### Scissor Testing

```c
// Only render to a specific rectangle
cfr_apply_scissor(renderer, 100, 100, 200, 200);
cfr_draw_elements(renderer);

// Disable scissor (use full viewport)
int w, h;
SDL_GetWindowSize(window, &w, &h);
cfr_apply_scissor(renderer, 0, 0, (float)w, (float)h);
```

### Custom Blend Modes

```c
CFR_RenderState state = cfr_render_state_defaults();
state.blend.enabled = true;
state.blend.rgb_src_blend_factor = CFR_BLENDFACTOR_SRC_ALPHA;
state.blend.rgb_dst_blend_factor = CFR_BLENDFACTOR_ONE;  // Additive blending
state.blend.rgb_op = CFR_BLEND_OP_ADD;

cfr_material_set_render_state(renderer, material, state);
```

## Platform-Specific Notes

### Windows
- Uses D3D11 or D3D12 via SDL3
- SPIR-V is cross-compiled by SDL3

### macOS
- Uses Metal via SDL3
- SPIR-V is cross-compiled by SDL3
- May need `SDL_WINDOW_METAL` flag

### Linux
- Uses Vulkan via SDL3
- Direct SPIR-V usage (no cross-compilation)
- Requires Vulkan drivers

## Performance Tips

1. **Minimize state changes** - Group draws by shader/material
2. **Reuse resources** - Don't create/destroy textures every frame
3. **Update buffers efficiently** - Use appropriate buffer sizes
4. **Batch similar geometry** - Combine into single meshes where possible

## Troubleshooting

### "SDL_CreateGPUDevice failed"
- Ensure you have GPU drivers installed
- Try different `SDL_GPU_SHADERFORMAT` options
- Check if window flags match backend (VULKAN/METAL/etc.)

### "Black screen"
- Verify shaders compile correctly to SPIR-V
- Check vertex attributes match shader inputs
- Ensure viewport is set correctly
- Verify blend state allows rendering

### "Crash on draw"
- Check mesh has valid data
- Ensure shader and material are valid
- Verify render pass is active (called `cfr_apply_canvas`)

## Further Reading

- [SDL3 GPU Documentation](https://wiki.libsdl.org/SDL3/)
- [SPIR-V Tooling](https://github.com/KhronosGroup/SPIRV-Tools)
- [Vulkan GLSL Reference](https://www.khronos.org/opengl/wiki/Core_Language_(GLSL))
