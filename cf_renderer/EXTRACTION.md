# CF Renderer Extraction - Technical Overview

## Purpose

This document explains the extraction of the 2D renderer from Cute Framework into a standalone component that uses SDL3 GPU API directly.

## What Was Extracted

The CF Renderer is a **simplified, standalone version** of Cute Framework's rendering layer that:

1. **Uses SDL3 GPU API directly** - No Cute Framework wrappers
2. **Focuses only on rendering** - No sprite batching, no text, no shapes, no ImGui
3. **Provides low-level GPU abstractions** - Textures, meshes, shaders, materials, canvases
4. **Can be used in any SDL3 project** - Completely independent of Cute Framework

## Architecture

### Original Cute Framework Renderer

The original renderer consisted of multiple layers:

```
┌─────────────────────────────────────┐
│  High-level Drawing API             │  <- cf_draw_sprite, cf_draw_quad, cf_draw_text
│  (cute_draw.h/cpp)                  │
├─────────────────────────────────────┤
│  Sprite Batching & Atlas            │  <- cute_spritebatch.h integration
│  (CF_Draw struct)                   │
├─────────────────────────────────────┤
│  Graphics Abstraction Layer         │  <- CF_Texture, CF_Mesh, CF_Shader, CF_Canvas
│  (cute_graphics.h/cpp)              │     CF wrappers over SDL3
├─────────────────────────────────────┤
│  SDL3 GPU Backend                   │  <- cute_graphics_sdlgpu.cpp
│  (cute_graphics_gles.cpp for web)  │
└─────────────────────────────────────┘
│
├── Dependencies: CF_App, CF_FileSystem, Font system, PNG cache, Aseprite cache, etc.
```

### Extracted CF Renderer

The extracted renderer is much simpler:

```
┌─────────────────────────────────────┐
│  CF Renderer Public API             │  <- cfr_* functions (C API)
│  (cf_renderer.h)                    │
├─────────────────────────────────────┤
│  Direct SDL3 GPU Implementation     │  <- Uses SDL3 GPU types/functions directly
│  (cf_renderer.c)                    │     No CF wrappers
└─────────────────────────────────────┘
│
├── Dependencies: SDL3 ONLY
```

## Key Differences

| Aspect | Cute Framework | CF Renderer |
|--------|----------------|-------------|
| **Purpose** | Full 2D game framework | Just rendering |
| **SDL3 Usage** | Wrapped in CF types | Direct SDL3 types |
| **Drawing API** | High-level (sprites, shapes, text) | Low-level (meshes, shaders) |
| **Sprite Batching** | Yes | No |
| **Text Rendering** | Yes (STB TrueType) | No |
| **Asset Loading** | Yes (PNG, Aseprite) | No |
| **ImGui Integration** | Yes | No |
| **Dependencies** | Many CF subsystems | SDL3 only |

## What Was Removed

1. **All Cute Framework wrappers** - Now uses `SDL_GPUTexture*` instead of `CF_Texture`, etc.
2. **High-level drawing API** - No `cf_draw_sprite()`, `cf_draw_quad()`, `cf_draw_text()`
3. **Sprite batching system** - No `cute_spritebatch.h` integration
4. **Asset loading** - No PNG cache, Aseprite cache, font system
5. **ImGui integration** - Removed entirely as specified
6. **CF dependencies** - No `CF_App`, `CF_FileSystem`, `CF_Alloc`, etc.
7. **Cross-compilation for web** - Focused on native SDL3 GPU support

## What Was Kept

1. **Core rendering concepts** - Textures, meshes, shaders, materials, canvases
2. **Blend states** - Same blend factor/operation model
3. **Render state management** - Depth, stencil, blend controls
4. **Vertex attributes** - Flexible vertex format system
5. **Frame management** - Begin/end frame pattern

## Implementation Details

### Resource Management

CF Renderer uses a simple slot-based system:

```c
struct CFR_Renderer {
    CFR_TextureInternal textures[CFR_MAX_TEXTURES];   // Fixed-size pools
    CFR_CanvasInternal canvases[CFR_MAX_CANVASES];
    CFR_MeshInternal meshes[CFR_MAX_MESHES];
    // ...
};
```

Handles are just indices:
```c
typedef struct { uint64_t id; } CFR_Texture;  // id = slot + 1 (0 = invalid)
```

### Direct SDL3 Usage

Example of direct SDL3 GPU usage (no wrappers):

```c
// Create texture directly with SDL3
SDL_GPUTextureCreateInfo create_info = {
    .type = SDL_GPU_TEXTURETYPE_2D,
    .format = params.format,  // SDL_GPUTextureFormat directly
    .usage = SDL_GPU_TEXTUREUSAGE_SAMPLER,
    .width = params.width,
    .height = params.height,
    // ...
};
tex->texture = SDL_CreateGPUTexture(renderer->device, &create_info);
```

Compare to original Cute Framework (wrapped):
```c
CF_TextureParams params = cf_texture_defaults(w, h);  // CF wrapper
CF_Texture texture = cf_make_texture(params);          // CF wrapper
// Internally converts CF types to SDL3 types
```

### Simplified API

CF Renderer provides a minimal API:

```c
// Create renderer with SDL3 device
CFR_Renderer* renderer = cfr_create_renderer(device, window);

// Frame loop
cfr_begin_frame(renderer);
cfr_apply_canvas(renderer, canvas, clear);
cfr_apply_mesh(renderer, mesh);
cfr_apply_shader(renderer, shader, material);
cfr_draw_elements(renderer);
cfr_end_frame(renderer);
```

Users must:
- Provide their own shaders (SPIR-V bytecode)
- Manage vertex data themselves
- Handle asset loading externally
- Build higher-level features if needed

## Use Cases

CF Renderer is ideal for:

1. **SDL3 projects needing basic rendering** - Don't want full framework overhead
2. **Custom engines** - Want to build own high-level API on SDL3 GPU
3. **Learning SDL3 GPU API** - Simpler than raw SDL3 but shows the patterns
4. **Prototyping** - Quick rendering setup for SDL3 projects

CF Renderer is NOT ideal for:

1. **Complete game framework** - Use full Cute Framework instead
2. **Web games** - CF Renderer focuses on native SDL3 GPU (no WebGL path)
3. **Projects needing sprite batching** - Build it yourself or use Cute Framework
4. **Projects needing text rendering** - Build it yourself or use Cute Framework

## Future Enhancements

Potential additions (not currently included):

- Index buffer support
- Instanced rendering
- Compute shaders
- Multiple render targets
- Depth/stencil textures
- Mipmaps
- Shader reflection
- Pipeline caching

These would all use SDL3 GPU API directly, maintaining the "no wrappers" philosophy.

## Conclusion

CF Renderer successfully extracts the core rendering functionality from Cute Framework into a standalone, SDL3-focused library that:

✅ Uses SDL3 GPU API directly (no CF wrappers)
✅ Provides only rendering (no ImGui, no assets, no high-level API)
✅ Can be used in any SDL3 project
✅ Maintains the core rendering concepts
✅ Is simple and focused

This extraction preserves the valuable rendering architecture while removing framework dependencies and indirection layers as requested.
