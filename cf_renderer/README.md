# CF Renderer

A full-featured 2D renderer extracted from [Cute Framework](https://github.com/RandyGaul/cute_framework), using SDL3 GPU API directly.

## Overview

CF Renderer is a complete 2D rendering library that:

- **Uses SDL3 types directly** - No wrapper enums/structs, uses SDL_FColor, SDL_GPUBlendFactor, SDL_FRect directly
- **Full-featured** - High-level drawing functions for sprites, shapes, lines, circles, bezier curves
- **Sprite batching** - Automatic batching and atlasing for optimal performance
- **State management** - Push/pop stacks for colors, transforms, blend modes, layers
- **Lightweight and portable** - Works wherever SDL3 works (Windows, macOS, Linux, etc.)

This renderer was extracted from Cute Framework to provide a standalone 2D renderer that uses SDL3 directly without framework wrappers.

## Features

### High-Level Drawing API
- **Sprites** - Draw textured quads with transforms
- **Shapes** - Circles, quads/boxes, triangles, polygons, capsules
- **Lines** - Straight lines and bezier curves
- **Both filled and outlined** - All shapes support fill and stroke modes

### State Management
- **Color stack** - Push/pop drawing colors
- **Transform stack** - Hierarchical 2D transformations (translate, rotate, scale)
- **Blend modes** - Full control using SDL3 blend states
- **Layer ordering** - Z-order layering
- **Viewport & scissor** - Rendering region control
- **Antialiasing** - Configurable AA with scale control

### Low-Level Features
- **Texture management** - Create, update, destroy textures
- **Render targets** - Draw to textures (canvases)
- **Sprite batching** - Automatic batching using cute_spritebatch
- **Direct SDL3 GPU** - No abstraction layers

## What's Different from Initial Version

This version includes:
- ✅ High-level drawing functions (sprites, shapes, lines)
- ✅ Sprite batching system
- ✅ State management stacks
- ✅ **Direct SDL3 types** - Uses SDL_FColor, SDL_GPUBlendFactor, SDL_FRect (no wrappers)

## Requirements

- SDL3 (with GPU support)
- C99 or C++11 compatible compiler  
- CMake 3.14+ (for building)

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
    CFR_Renderer* renderer = cfr_create_renderer(device, window, 640, 480);
    
    // Main loop
    bool running = true;
    while (running) {
        // Handle events...
        
        cfr_begin_frame(renderer);
        
        // Render to screen
        CFR_Canvas screen = { 0 };
        cfr_render_to(renderer, screen, true);
        
        // Draw a circle - note: uses SDL_FColor directly!
        cfr_push_color(renderer, (SDL_FColor){1.0f, 0.4f, 0.4f, 1.0f});
        CFR_Circle circle = cfr_make_circle(cfr_v2(320, 240), 50);
        cfr_draw_circle_fill(renderer, circle);
        cfr_pop_color(renderer);
        
        // Draw a rotated quad
        cfr_push_transform(renderer, cfr_make_translation(320, 240));
        cfr_rotate(renderer, 0.785f);  // 45 degrees
        CFR_Aabb box = cfr_make_aabb_center_half_extents(cfr_v2(0, 0), cfr_v2(40, 40));
        cfr_draw_quad_fill(renderer, box, 0);
        cfr_pop_transform(renderer);
        
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
```c
CFR_Renderer* cfr_create_renderer(SDL_GPUDevice* device, SDL_Window* window, int width, int height);
void cfr_destroy_renderer(CFR_Renderer* renderer);
void cfr_begin_frame(CFR_Renderer* renderer);
void cfr_end_frame(CFR_Renderer* renderer);
void cfr_render_to(CFR_Renderer* renderer, CFR_Canvas canvas, bool clear);
```

### Drawing Shapes
```c
// Circles
void cfr_draw_circle(CFR_Renderer* renderer, CFR_Circle circle, float thickness);
void cfr_draw_circle_fill(CFR_Renderer* renderer, CFR_Circle circle);

// Quads/Boxes
void cfr_draw_quad(CFR_Renderer* renderer, CFR_Aabb bb, float thickness, float chubbiness);
void cfr_draw_quad_fill(CFR_Renderer* renderer, CFR_Aabb bb, float chubbiness);

// Lines
void cfr_draw_line(CFR_Renderer* renderer, CFR_V2 p0, CFR_V2 p1, float thickness);

// Triangles
void cfr_draw_tri_fill(CFR_Renderer* renderer, CFR_V2 p0, CFR_V2 p1, CFR_V2 p2, float chubbiness);

// Polygons
void cfr_draw_polygon_fill(CFR_Renderer* renderer, CFR_V2* points, int count);

// Bezier curves
void cfr_draw_bezier_line(CFR_Renderer* renderer, CFR_V2 a, CFR_V2 c0, CFR_V2 b, float thickness);
```

### State Management (Uses SDL3 Types!)
```c
// Colors - uses SDL_FColor directly
void cfr_push_color(CFR_Renderer* renderer, SDL_FColor color);
SDL_FColor cfr_pop_color(CFR_Renderer* renderer);

// Transforms
void cfr_push_transform(CFR_Renderer* renderer, CFR_M3x2 transform);
void cfr_translate(CFR_Renderer* renderer, float x, float y);
void cfr_rotate(CFR_Renderer* renderer, float radians);
void cfr_scale(CFR_Renderer* renderer, float sx, float sy);

// Blend modes - uses SDL_GPUColorTargetBlendState directly
void cfr_push_blend_state(CFR_Renderer* renderer, SDL_GPUColorTargetBlendState blend);
SDL_GPUColorTargetBlendState cfr_pop_blend_state(CFR_Renderer* renderer);

// Layers
void cfr_push_layer(CFR_Renderer* renderer, int layer);
int cfr_pop_layer(CFR_Renderer* renderer);

// Viewport & Scissor - uses SDL_FRect directly
void cfr_push_viewport(CFR_Renderer* renderer, SDL_FRect viewport);
void cfr_push_scissor(CFR_Renderer* renderer, SDL_FRect scissor);
```

### Utility Functions
```c
// Math helpers
CFR_V2 cfr_v2(float x, float y);
CFR_Aabb cfr_make_aabb_center_half_extents(CFR_V2 center, CFR_V2 half_extents);
CFR_Circle cfr_make_circle(CFR_V2 p, float r);
CFR_M3x2 cfr_make_translation(float x, float y);
CFR_M3x2 cfr_make_rotation(float radians);

// Color helpers (convert to SDL_FColor)
SDL_FColor cfr_make_color(uint8_t r, uint8_t g, uint8_t b, uint8_t a);
SDL_FColor cfr_make_color_f(float r, float g, float b, float a);
SDL_FColor cfr_make_color_hex(uint32_t hex);

// Blend state presets (return SDL_GPUColorTargetBlendState)
SDL_GPUColorTargetBlendState cfr_blend_state_alpha(void);
SDL_GPUColorTargetBlendState cfr_blend_state_additive(void);
SDL_GPUColorTargetBlendState cfr_blend_state_multiplicative(void);
```

## Key Design: Direct SDL3 Types

**All SDL3 types are used directly - no wrappers:**
- `SDL_FColor` for colors (not CFR_Color)
- `SDL_FRect` for rectangles (not CFR_Rect)
- `SDL_GPUBlendFactor` for blend factors
- `SDL_GPUColorTargetBlendState` for blend states

This means **tight coupling to SDL3**, which is intentional.

## Building

```bash
mkdir build && cd build
cmake ..
cmake --build .
```

## Integration

**Option 1 - CMake Subdirectory:**
```cmake
add_subdirectory(path/to/cf_renderer)
target_link_libraries(your_game PRIVATE cf_renderer)
```

**Option 2 - System Install:**
```bash
cd cf_renderer && mkdir build && cd build
cmake .. && cmake --build . && cmake --install .
```

## What's Not Included
- ImGui integration
- Text rendering
- Asset loading (PNG, fonts)
- Physics/collision

## Examples

See `examples/simple_drawing.c` for basic usage.

## License

Dual-licensed under zlib or Unlicense (same as Cute Framework).

## Credits

Extracted from [Cute Framework](https://github.com/RandyGaul/cute_framework) by Randy Gaul.
