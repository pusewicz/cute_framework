# CuteRenderer

A Swift 2D rendering library extracted from [Cute Framework](https://github.com/RandyGaul/cute_framework), providing sprite and shape rendering capabilities with SDL3 GPU backend support.

## Features

- **High-level 2D Drawing API**: Draw shapes (quads, circles, capsules, triangles, lines, polygons) with SDF-based antialiasing
- **Sprite System**: Full animation support with Aseprite file compatibility, 9-slice rendering
- **Push/Pop State Management**: Manage colors, layers, shaders, scissors, and viewports with stack-based state
- **Low-level Graphics API**: Direct access to textures, canvases, shaders, materials, and meshes
- **Swift-native API**: Clean Swift API with API notes for seamless C interop

## Requirements

- Swift 5.9+
- macOS 14+, iOS 17+, tvOS 17+, or visionOS 1+
- SDL3 (for GPU backend)

## Installation

Add CuteRenderer to your `Package.swift`:

```swift
dependencies: [
    .package(path: "path/to/CuteRenderer")
]
```

## Quick Start

```swift
import CuteRenderer

// Initialize the renderer
CuteRenderer.initialize()

// Drawing shapes
Draw.pushColor(.red)
Draw.circleFill(center: Vector2(x: 100, y: 100), radius: 50)
Draw.line(from: Vector2(x: 0, y: 0), to: Vector2(x: 200, y: 200), thickness: 2)
Draw.popColor()

// Drawing sprites
if var sprite = Sprite.fromAseprite(path: "player.ase") {
    sprite.play("walk")
    sprite.update(deltaTime: 1.0/60.0)
    sprite.draw()
}

// Flush to GPU
Draw.flush()
```

## Architecture

### Math Types
- `Vector2` - 2D vector
- `SinCos` - Precomputed rotation
- `Transform` - Position and rotation
- `Matrix3x2` - Full 2D transformation with scale
- `AABB` - Axis-aligned bounding box
- `Circle`, `Capsule`, `Ray` - Geometric primitives

### Color Types
- `Color` - Float RGBA (0-1)
- `Pixel` - Byte RGBA (0-255)

### Graphics Types
- `Texture` - GPU texture with various pixel formats
- `Canvas` - Render target for off-screen rendering
- `Shader` - GLSL vertex and fragment shader programs
- `Material` - Shader uniforms and texture bindings
- `Mesh` - Vertex and index buffers

### Draw API
- Push/pop state stacks (color, layer, antialias, shader, scissor, viewport)
- Shape drawing (quad, circle, capsule, triangle, line, polyline, polygon)
- Sprite drawing with animation support

### Sprite System
- Animation frames with timing
- Multiple animations per sprite
- Play directions (forwards, backwards, pingpong)
- 9-slice rendering for UI

## C API Bridge

CuteRenderer includes a C API bridge (`CuteRendererC`) for integrating with native SDL3 GPU code. API notes provide Swift-friendly naming:

```swift
// C function: cr_draw_circle(center, radius, thickness)
// Swift becomes: CuteDraw.circle(center:radius:thickness:)
```

## License

This library is based on Cute Framework, which is dual-licensed under zlib and Unlicense.
