# CF Renderer

A standalone 2D renderer extracted from [Cute Framework](https://github.com/RandyGaul/cute_framework), built directly on SDL3's GPU API.

## Overview

CF Renderer is a minimal, focused 2D rendering library that:

- **Uses SDL3 GPU API directly** - No framework wrappers, tight coupling to SDL3
- **Lightweight and portable** - Works wherever SDL3 works (Windows, macOS, Linux, etc.)
- **Simple and focused** - Just the renderer, nothing else
- **No dependencies** - Only requires SDL3

This renderer was extracted from Cute Framework to provide a standalone component that can be used in any SDL3 project without requiring the full framework.

## Features

- Direct SDL3 GPU API usage (Vulkan, Metal, D3D12, D3D11 backends via SDL3)
- Texture management
- Render targets (canvases)
- Mesh/vertex buffer management
- Shader support (SPIR-V bytecode)
- Material system with blend states
- Viewport and scissor control

## What's NOT Included

- ImGui integration (removed as per extraction requirements)
- Asset loading (PNG, Aseprite, fonts, etc.)
- High-level drawing API (sprites, shapes, text)
- Sprite batching
- Application framework
- Input handling
- Audio system
- File system utilities

This is **just the renderer** - you bring everything else.

## Requirements

- SDL3 (with GPU support)
- C99 or C++11 compatible compiler
- CMake 3.14+ (for building)

## Building

```bash
mkdir build
cd build
cmake ..
cmake --build .
```

## Quick Start

```c
#include "cf_renderer.h"
#include <SDL3/SDL.h>

int main(int argc, char* argv[])
{
    // Initialize SDL and create window
    SDL_Init(SDL_INIT_VIDEO);
    SDL_Window* window = SDL_CreateWindow("My Game", 640, 480, SDL_WINDOW_VULKAN);
    
    // Create GPU device
    SDL_GPUDevice* device = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV, true, NULL);
    SDL_ClaimWindowForGPUDevice(device, window);
    
    // Create renderer
    CFR_Renderer* renderer = cfr_create_renderer(device, window);
    
    // Create resources (mesh, shader, material)
    // ... your code here ...
    
    // Main loop
    while (running) {
        cfr_begin_frame(renderer);
        
        // Render to screen
        CFR_Canvas screen = { 0 };
        cfr_apply_canvas(renderer, screen, true);
        
        // Draw your content
        // ...
        
        cfr_end_frame(renderer);
    }
    
    // Cleanup
    cfr_destroy_renderer(renderer);
    SDL_DestroyGPUDevice(device);
    SDL_DestroyWindow(window);
    SDL_Quit();
    
    return 0;
}
```

## API Overview

### Renderer Lifecycle
- `cfr_create_renderer()` - Create renderer instance
- `cfr_destroy_renderer()` - Destroy renderer
- `cfr_begin_frame()` - Begin a new frame
- `cfr_end_frame()` - End frame and present

### Resources
- **Textures**: `cfr_make_texture()`, `cfr_texture_update()`, `cfr_destroy_texture()`
- **Canvases**: `cfr_make_canvas()`, `cfr_canvas_get_texture()`, `cfr_destroy_canvas()`
- **Meshes**: `cfr_make_mesh()`, `cfr_mesh_update_vertex_data()`, `cfr_destroy_mesh()`
- **Shaders**: `cfr_make_shader()`, `cfr_destroy_shader()`
- **Materials**: `cfr_make_material()`, `cfr_material_set_texture()`, `cfr_material_set_uniform()`, `cfr_destroy_material()`

### Drawing
- `cfr_apply_canvas()` - Set render target
- `cfr_apply_mesh()` - Bind mesh
- `cfr_apply_shader()` - Bind shader and material
- `cfr_apply_viewport()` - Set viewport
- `cfr_apply_scissor()` - Set scissor rectangle
- `cfr_draw_elements()` - Draw

## Design Philosophy

This renderer follows the principle of **direct SDL3 usage**:

1. **No Cute Framework wrappers** - Uses SDL3 types and functions directly
2. **Minimal abstraction** - Thin layer over SDL3 GPU API
3. **Tight SDL3 coupling** - This is intentional, not a limitation
4. **Single purpose** - Just rendering, nothing else

If you need higher-level features (sprite batching, shape drawing, text rendering), either:
- Use the full Cute Framework
- Build your own on top of CF Renderer
- Use complementary libraries

## Differences from Cute Framework

The original Cute Framework renderer:
- Was part of a larger game framework
- Included high-level 2D drawing API
- Integrated with CF's asset system
- Had sprite batching and atlasing
- Supported text rendering with fonts
- Included ImGui integration

CF Renderer:
- Is standalone and minimal
- Uses SDL3 GPU API directly
- Requires you to provide shaders as SPIR-V bytecode
- Requires you to manage vertices and draw calls
- No built-in asset loading
- No ImGui integration

## Examples

See the `examples/` directory for sample code:
- `simple_triangle.c` - Basic triangle rendering

## License

Dual-licensed under zlib or Unlicense (same as Cute Framework).

## Credits

Extracted from [Cute Framework](https://github.com/RandyGaul/cute_framework) by Randy Gaul.
