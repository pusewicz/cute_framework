// CuteRenderer - A Swift 2D Renderer based on Cute Framework
//
// This library provides a pure Swift implementation of a 2D renderer
// with sprite and shape drawing capabilities, originally extracted from
// the Cute Framework (https://github.com/RandyGaul/cute_framework).
//
// Features:
// - High-level 2D drawing API for shapes (quads, circles, capsules, triangles, lines)
// - Sprite rendering with animation support
// - SDF-based antialiasing for smooth shapes
// - Layer-based rendering with push/pop state management
// - Low-level graphics API for custom rendering

// MARK: - Re-exports

// Math types
@_exported import struct CuteRenderer.Vector2
@_exported import struct CuteRenderer.SinCos
@_exported import struct CuteRenderer.Transform
@_exported import struct CuteRenderer.Matrix3x2
@_exported import struct CuteRenderer.AABB
@_exported import struct CuteRenderer.Circle
@_exported import struct CuteRenderer.Capsule
@_exported import struct CuteRenderer.Ray
@_exported import struct CuteRenderer.RaycastHit
@_exported import struct CuteRenderer.Rect

// Color types
@_exported import struct CuteRenderer.Color
@_exported import struct CuteRenderer.Pixel

// Graphics types
@_exported import enum CuteRenderer.PixelFormat
@_exported import enum CuteRenderer.TextureFilter
@_exported import enum CuteRenderer.MipFilter
@_exported import enum CuteRenderer.WrapMode
@_exported import struct CuteRenderer.TextureUsage
@_exported import struct CuteRenderer.TextureParams
@_exported import class CuteRenderer.Texture
@_exported import struct CuteRenderer.CanvasParams
@_exported import class CuteRenderer.Canvas
@_exported import class CuteRenderer.Shader
@_exported import class CuteRenderer.Material
@_exported import class CuteRenderer.Mesh

// Draw types
@_exported import class CuteRenderer.DrawContext

// Sprite types
@_exported import enum CuteRenderer.PlayDirection
@_exported import struct CuteRenderer.AnimationFrame
@_exported import struct CuteRenderer.Animation
@_exported import struct CuteRenderer.SpriteSlice
@_exported import struct CuteRenderer.Sprite

// MARK: - Version

/// The CuteRenderer version.
public struct CuteRendererVersion {
    /// Major version number.
    public static let major = 1

    /// Minor version number.
    public static let minor = 0

    /// Patch version number.
    public static let patch = 0

    /// Version string.
    public static var string: String {
        return "\(major).\(minor).\(patch)"
    }
}

// MARK: - Renderer

/// The main renderer interface.
///
/// CuteRenderer provides a simple, immediate-mode 2D rendering API.
///
/// ## Overview
///
/// Use the ``Draw`` global to access drawing functions:
///
/// ```swift
/// // Set a color for drawing
/// Draw.pushColor(.red)
///
/// // Draw shapes
/// Draw.circleFill(center: Vector2(100, 100), radius: 50)
/// Draw.line(from: Vector2(0, 0), to: Vector2(200, 200), thickness: 2)
///
/// // Draw sprites
/// if let sprite = Sprite.fromPNG(path: "player.png") {
///     Draw.sprite(sprite)
/// }
///
/// // Restore color
/// Draw.popColor()
///
/// // Flush to GPU
/// Draw.flush()
/// ```
///
/// ## Topics
///
/// ### Drawing
/// - ``Draw``
/// - ``DrawContext``
///
/// ### Shapes
/// - ``drawQuad(_:thickness:chubbiness:)``
/// - ``drawCircle(center:radius:thickness:)``
/// - ``drawLine(from:to:thickness:)``
///
/// ### Sprites
/// - ``Sprite``
/// - ``drawSprite(_:)``
///
/// ### Graphics
/// - ``Texture``
/// - ``Canvas``
/// - ``Shader``
/// - ``Material``
/// - ``Mesh``
public enum CuteRenderer {
    /// Initializes the renderer.
    ///
    /// Call this before using any rendering functions.
    public static func initialize() {
        // Initialize the renderer
    }

    /// Shuts down the renderer and releases resources.
    public static func shutdown() {
        // Cleanup
    }

    /// The current graphics backend being used.
    public static var backend: String {
        return "SDL3 GPU"
    }
}
