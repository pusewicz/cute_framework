# CuteRenderer

A thin Swift wrapper over SDL3 GPU for 2D rendering, inspired by [Cute Framework](https://github.com/RandyGaul/cute_framework).

## Features

- **Direct SDL3 GPU Access**: Use SDL3 GPU types directly from Swift with convenient extensions
- **High-level 2D Drawing API**: Draw shapes with SDF-based antialiasing
- **Sprite System**: Animation support with Aseprite compatibility
- **Push/Pop State Management**: Manage colors, layers, and transforms with stack-based state
- **Zero Abstraction Cost**: SDL3 types used directly, no intermediate wrapper classes

## Requirements

- Swift 5.9+
- macOS 14+, iOS 17+, or tvOS 17+
- SDL3 (installed via Homebrew or system package manager)

## Installation

Add CuteRenderer to your `Package.swift`:

```swift
dependencies: [
    .package(path: "path/to/CuteRenderer")
]
```

Make sure SDL3 is installed:
```bash
# macOS
brew install sdl3

# Ubuntu/Debian
apt install libsdl3-dev
```

## Quick Start

```swift
import CuteRenderer

// Initialize with an SDL window
let renderer = Renderer.shared
renderer.initialize(window: mySDLWindow)

// Frame loop
if renderer.beginFrame() {
    if renderer.beginSwapchainRenderPass(clearColor: .black) {
        // Draw shapes
        renderer.pushColor(.red)
        renderer.circleFill(center: Vector2(x: 100, y: 100), radius: 50)
        renderer.line(from: .zero, to: Vector2(x: 200, y: 200), thickness: 2)
        renderer.popColor()

        // Draw sprites
        if var sprite = Sprite.fromAseprite(path: "player.ase") {
            sprite.play("walk")
            sprite.update(deltaTime: 1.0/60.0)
            renderer.sprite(sprite)
        }

        renderer.endRenderPass()
    }
    renderer.endFrame()
}
```

## Architecture

### Using SDL3 Directly

CuteRenderer exposes SDL3 GPU types directly. You can use SDL3 functions alongside the Swift API:

```swift
import CuteRenderer  // Also exports CSDL3

// Create textures using SDL3
let texture = SDL_CreateGPUTexture(renderer.device, &textureInfo)

// Use Swift extensions for convenience
let texture2 = renderer.device?.createTexture(width: 256, height: 256)
```

### Math Types
- `Vector2` - 2D vector with operators and geometric functions
- `SinCos` - Precomputed sin/cos for efficient rotation
- `Transform` - Position + rotation (no scale)
- `Matrix3x2` - Full 2D transformation with scale
- `AABB`, `Circle`, `Capsule`, `Ray`, `Rect` - Geometric primitives

### Color Types
- `Color` - Float RGBA (0-1) with HSV conversion and blending
- `Pixel` - Byte RGBA (0-255) for texture data

### Renderer
The `Renderer` class wraps SDL3 GPU device and provides:
- Frame management (`beginFrame`, `endFrame`)
- Render pass control (`beginSwapchainRenderPass`, `endRenderPass`)
- State stacks (color, layer, antialias, vertex attributes)
- Shape drawing (quad, circle, capsule, triangle, line, polyline, polygon, bezier, arrow)
- Sprite drawing with animation

### SDL3 Extensions
Swift extensions on SDL3 types for convenience:
```swift
// SDL_FColor from Color
let sdlColor = myColor.sdlColor

// Vector2 from SDL_FPoint
let vec = Vector2(sdlPoint)

// AABB from SDL_FRect
let box = AABB(sdlRect)
```

### Sprite System
- `Sprite` - Drawable with texture, animation, and transform
- `Animation` - Named sequence of frames with timing
- `PlayDirection` - forwards, backwards, pingpong
- 9-slice rendering for UI elements

## License

Inspired by Cute Framework, dual-licensed under zlib and Unlicense.
