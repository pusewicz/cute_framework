/// A vertex used by the drawing system.
///
/// Contains all data needed to render shapes and sprites with SDF-based antialiasing.
public struct DrawVertex: Hashable, Sendable {
    /// World space position.
    public var position: Vector2

    /// Homogeneous position (transformed by camera).
    public var transformedPosition: Vector2

    /// Number of shape vertices (for SDF).
    public var shapeVertexCount: Int

    /// Shape vertices for SDF rendering.
    public var shapeVertices: (Vector2, Vector2, Vector2, Vector2, Vector2, Vector2, Vector2, Vector2)

    /// UV coordinates.
    public var uv: Vector2

    /// Vertex color.
    public var color: Pixel

    /// Radius for circles/capsules or chubbiness for other shapes.
    public var radius: Float

    /// Stroke thickness (0 for filled shapes).
    public var stroke: Float

    /// Antialiasing factor.
    public var antialias: Float

    /// Shape type.
    public var type: UInt8

    /// Alpha value.
    public var alpha: UInt8

    /// Whether shape is filled.
    public var fill: UInt8

    /// Unused padding.
    public var unused: UInt8

    /// Custom attributes for shaders.
    public var attributes: Color

    /// Creates a draw vertex.
    public init(
        position: Vector2 = .zero,
        transformedPosition: Vector2 = .zero,
        uv: Vector2 = .zero,
        color: Pixel = .white
    ) {
        self.position = position
        self.transformedPosition = transformedPosition
        self.shapeVertexCount = 0
        self.shapeVertices = (.zero, .zero, .zero, .zero, .zero, .zero, .zero, .zero)
        self.uv = uv
        self.color = color
        self.radius = 0
        self.stroke = 0
        self.antialias = 1.5
        self.type = 0
        self.alpha = 255
        self.fill = 1
        self.unused = 0
        self.attributes = .clear
    }
}

/// A callback for modifying vertices before rendering.
public typealias VertexCallback = ([DrawVertex]) -> [DrawVertex]

/// The drawing context manages state for 2D rendering.
///
/// Use the shared `Draw` instance to draw shapes, sprites, and text.
/// All drawing operations are batched and rendered when ``flush()`` is called.
///
/// ## Topics
///
/// ### Getting the Draw Context
/// - ``shared``
///
/// ### State Stack
/// - ``pushColor(_:)``
/// - ``popColor()``
/// - ``pushLayer(_:)``
/// - ``popLayer()``
/// - ``pushAntialias(_:)``
/// - ``popAntialias()``
///
/// ### Drawing Shapes
/// - ``quad(_:thickness:chubbiness:)``
/// - ``quadFill(_:chubbiness:)``
/// - ``circle(center:radius:thickness:)``
/// - ``circleFill(center:radius:)``
/// - ``line(from:to:thickness:)``
/// - ``triangle(_:_:_:thickness:chubbiness:)``
/// - ``triangleFill(_:_:_:chubbiness:)``
///
/// ### Rendering
/// - ``flush()``
public final class DrawContext: @unchecked Sendable {
    /// The shared draw context.
    public static let shared = DrawContext()

    // MARK: - State Stacks

    private var colorStack: [Color] = [.white]
    private var layerStack: [Int] = [0]
    private var antialiasStack: [Bool] = [true]
    private var antialiasScaleStack: [Float] = [1.5]
    private var attributeStack: [Color] = [.clear]
    private var shaderStack: [Shader?] = [nil]
    private var scissorStack: [Rect?] = [nil]
    private var viewportStack: [Rect?] = [nil]

    /// Current camera transform.
    public var camera: Matrix3x2 = .identity

    /// Current canvas to draw to.
    public var canvas: Canvas?

    private init() {}

    // MARK: - Color State

    /// The current drawing color.
    public var color: Color {
        return colorStack.last ?? .white
    }

    /// Pushes a color onto the stack.
    @discardableResult
    public func pushColor(_ color: Color) -> Color {
        colorStack.append(color)
        return color
    }

    /// Pops the color stack and returns the popped value.
    @discardableResult
    public func popColor() -> Color {
        guard colorStack.count > 1 else { return colorStack.first ?? .white }
        return colorStack.removeLast()
    }

    /// Returns the current color without modifying the stack.
    public func peekColor() -> Color {
        return colorStack.last ?? .white
    }

    // MARK: - Layer State

    /// The current drawing layer.
    public var layer: Int {
        return layerStack.last ?? 0
    }

    /// Pushes a layer onto the stack.
    @discardableResult
    public func pushLayer(_ layer: Int) -> Int {
        layerStack.append(layer)
        return layer
    }

    /// Pops the layer stack.
    @discardableResult
    public func popLayer() -> Int {
        guard layerStack.count > 1 else { return layerStack.first ?? 0 }
        return layerStack.removeLast()
    }

    /// Returns the current layer.
    public func peekLayer() -> Int {
        return layerStack.last ?? 0
    }

    // MARK: - Antialias State

    /// Whether antialiasing is enabled.
    public var antialias: Bool {
        return antialiasStack.last ?? true
    }

    /// Pushes antialias state onto the stack.
    @discardableResult
    public func pushAntialias(_ enabled: Bool) -> Bool {
        antialiasStack.append(enabled)
        return enabled
    }

    /// Pops the antialias stack.
    @discardableResult
    public func popAntialias() -> Bool {
        guard antialiasStack.count > 1 else { return antialiasStack.first ?? true }
        return antialiasStack.removeLast()
    }

    /// The antialias scale factor.
    public var antialiasScale: Float {
        return antialiasScaleStack.last ?? 1.5
    }

    /// Pushes antialias scale onto the stack.
    @discardableResult
    public func pushAntialiasScale(_ scale: Float) -> Float {
        antialiasScaleStack.append(scale)
        return scale
    }

    /// Pops the antialias scale stack.
    @discardableResult
    public func popAntialiasScale() -> Float {
        guard antialiasScaleStack.count > 1 else { return antialiasScaleStack.first ?? 1.5 }
        return antialiasScaleStack.removeLast()
    }

    // MARK: - Vertex Attributes

    /// The current vertex attributes.
    public var vertexAttributes: Color {
        return attributeStack.last ?? .clear
    }

    /// Pushes vertex attributes onto the stack.
    @discardableResult
    public func pushVertexAttributes(_ attributes: Color) -> Color {
        attributeStack.append(attributes)
        return attributes
    }

    /// Pushes vertex attributes onto the stack.
    @discardableResult
    public func pushVertexAttributes(r: Float, g: Float, b: Float, a: Float) -> Color {
        return pushVertexAttributes(Color(r: r, g: g, b: b, a: a))
    }

    /// Pops the vertex attributes stack.
    @discardableResult
    public func popVertexAttributes() -> Color {
        guard attributeStack.count > 1 else { return attributeStack.first ?? .clear }
        return attributeStack.removeLast()
    }

    // MARK: - Shader State

    /// The current custom shader (nil = default).
    public var shader: Shader? {
        return shaderStack.last ?? nil
    }

    /// Pushes a shader onto the stack.
    public func pushShader(_ shader: Shader?) {
        shaderStack.append(shader)
    }

    /// Pops the shader stack.
    @discardableResult
    public func popShader() -> Shader? {
        guard shaderStack.count > 1 else { return shaderStack.first ?? nil }
        return shaderStack.removeLast()
    }

    // MARK: - Scissor State

    /// The current scissor rect (nil = no scissor).
    public var scissor: Rect? {
        return scissorStack.last ?? nil
    }

    /// Pushes a scissor rect onto the stack.
    public func pushScissor(_ rect: Rect) {
        scissorStack.append(rect)
    }

    /// Pushes a disabled scissor onto the stack.
    public func pushScissorOff() {
        scissorStack.append(nil)
    }

    /// Pops the scissor stack.
    @discardableResult
    public func popScissor() -> Rect? {
        guard scissorStack.count > 1 else { return scissorStack.first ?? nil }
        return scissorStack.removeLast()
    }

    // MARK: - Viewport State

    /// The current viewport (nil = full canvas).
    public var viewport: Rect? {
        return viewportStack.last ?? nil
    }

    /// Pushes a viewport onto the stack.
    public func pushViewport(_ rect: Rect) {
        viewportStack.append(rect)
    }

    /// Pushes a default viewport onto the stack.
    public func pushViewportOff() {
        viewportStack.append(nil)
    }

    /// Pops the viewport stack.
    @discardableResult
    public func popViewport() -> Rect? {
        guard viewportStack.count > 1 else { return viewportStack.first ?? nil }
        return viewportStack.removeLast()
    }

    // MARK: - Vertex Callback

    private var vertexCallback: VertexCallback?

    /// Sets a callback for modifying vertices before rendering.
    public func setVertexCallback(_ callback: VertexCallback?) {
        self.vertexCallback = callback
    }

    // MARK: - Rendering

    /// Flushes all pending draw commands to the GPU.
    public func flush() {
        // In real implementation, this would batch and render all pending draws
    }

    /// Resets all state to defaults.
    public func reset() {
        colorStack = [.white]
        layerStack = [0]
        antialiasStack = [true]
        antialiasScaleStack = [1.5]
        attributeStack = [.clear]
        shaderStack = [nil]
        scissorStack = [nil]
        viewportStack = [nil]
        camera = .identity
        canvas = nil
        vertexCallback = nil
    }
}
