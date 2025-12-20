/// Sample count for MSAA (multisample anti-aliasing).
public enum SampleCount: Int, Hashable, Codable, Sendable {
    /// No multisampling (1 sample).
    case x1 = 1

    /// 2x multisampling.
    case x2 = 2

    /// 4x multisampling.
    case x4 = 4

    /// 8x multisampling.
    case x8 = 8
}

/// Parameters for creating a canvas (render target).
public struct CanvasParams: Hashable, Sendable {
    /// The width in pixels.
    public var width: Int

    /// The height in pixels.
    public var height: Int

    /// The color format.
    public var colorFormat: PixelFormat

    /// The depth format (nil for no depth buffer).
    public var depthFormat: PixelFormat?

    /// Sample count for MSAA.
    public var sampleCount: SampleCount

    /// Whether to use depth testing.
    public var depthEnabled: Bool

    /// Whether to use stencil testing.
    public var stencilEnabled: Bool

    /// Creates canvas parameters with sensible defaults.
    public init(
        width: Int,
        height: Int,
        colorFormat: PixelFormat = .r8g8b8a8Unorm,
        depthFormat: PixelFormat? = nil,
        sampleCount: SampleCount = .x1,
        depthEnabled: Bool = false,
        stencilEnabled: Bool = false
    ) {
        self.width = width
        self.height = height
        self.colorFormat = colorFormat
        self.depthFormat = depthFormat
        self.sampleCount = sampleCount
        self.depthEnabled = depthEnabled
        self.stencilEnabled = stencilEnabled
    }

    /// Returns parameters for a simple color-only canvas.
    public static func color(width: Int, height: Int) -> CanvasParams {
        return CanvasParams(width: width, height: height)
    }

    /// Returns parameters for a canvas with depth buffer.
    public static func colorDepth(width: Int, height: Int) -> CanvasParams {
        return CanvasParams(
            width: width,
            height: height,
            depthFormat: .d24Unorm,
            depthEnabled: true
        )
    }
}

/// A canvas (render target) for off-screen rendering.
///
/// Canvases allow you to render to a texture instead of the screen.
/// This is useful for post-processing effects, UI rendering, and more.
///
/// ## Topics
///
/// ### Creating Canvases
/// - ``init(params:)``
/// - ``init(width:height:)``
///
/// ### Properties
/// - ``width``
/// - ``height``
/// - ``size``
/// - ``texture``
///
/// ### Rendering
/// - ``clear(color:)``
/// - ``clear(color:depth:)``
public final class Canvas: Hashable, Sendable {
    /// Internal handle (would be CF_Canvas in real implementation).
    internal let handle: UInt64

    /// The canvas parameters.
    public let params: CanvasParams

    /// The underlying texture (for sampling after rendering).
    public let texture: Texture

    /// The width in pixels.
    public var width: Int { params.width }

    /// The height in pixels.
    public var height: Int { params.height }

    /// The size as a Vector2.
    public var size: Vector2 {
        Vector2(x: Float(width), y: Float(height))
    }

    /// Creates a canvas with the specified parameters.
    public init(params: CanvasParams) {
        self.params = params
        self.handle = UInt64.random(in: 1...UInt64.max)

        // Create the backing texture
        self.texture = Texture(params: TextureParams(
            width: params.width,
            height: params.height,
            pixelFormat: params.colorFormat,
            usage: [.sampler, .colorTarget]
        ))
    }

    /// Creates a simple canvas with the specified dimensions.
    public convenience init(width: Int, height: Int) {
        self.init(params: CanvasParams(width: width, height: height))
    }

    /// Clears the canvas with a color.
    public func clear(color: Color) {
        // In real implementation, this would call cf_clear_color
    }

    /// Clears the canvas with a color and depth value.
    public func clear(color: Color, depth: Float) {
        // In real implementation, this would call cf_clear_color_and_depth
    }

    // MARK: - Hashable

    public static func == (lhs: Canvas, rhs: Canvas) -> Bool {
        return lhs.handle == rhs.handle
    }

    public func hash(into hasher: inout Hasher) {
        hasher.combine(handle)
    }
}
