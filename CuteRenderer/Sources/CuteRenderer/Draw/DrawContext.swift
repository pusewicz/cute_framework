import CSDL3

/// A vertex used by the drawing system.
///
/// Contains all data needed to render shapes and sprites with SDF-based antialiasing.
public struct DrawVertex: Hashable, Sendable {
    /// World space position.
    public var position: Vector2

    /// Homogeneous position (transformed by camera).
    public var transformedPosition: Vector2

    /// Number of shape vertices (for SDF).
    public var shapeVertexCount: Int32

    /// Shape vertices for SDF rendering (max 8).
    public var shape0, shape1, shape2, shape3: Vector2
    public var shape4, shape5, shape6, shape7: Vector2

    /// UV coordinates.
    public var uv: Vector2

    /// Vertex color.
    public var color: Pixel

    /// Radius for circles/capsules or chubbiness for other shapes.
    public var radius: Float

    /// Stroke thickness (0 for filled shapes).
    public var stroke: Float

    /// Antialiasing factor.
    public var aa: Float

    /// Shape type.
    public var type: UInt8

    /// Alpha value.
    public var alpha: UInt8

    /// Whether shape is filled.
    public var fill: UInt8

    /// Padding.
    public var _pad: UInt8

    /// Custom attributes for shaders.
    public var attributes: Color

    /// Creates a draw vertex with default values.
    public init() {
        self.position = .zero
        self.transformedPosition = .zero
        self.shapeVertexCount = 0
        self.shape0 = .zero; self.shape1 = .zero; self.shape2 = .zero; self.shape3 = .zero
        self.shape4 = .zero; self.shape5 = .zero; self.shape6 = .zero; self.shape7 = .zero
        self.uv = .zero
        self.color = .white
        self.radius = 0
        self.stroke = 0
        self.aa = 1.5
        self.type = 0
        self.alpha = 255
        self.fill = 1
        self._pad = 0
        self.attributes = .clear
    }
}

/// The drawing context manages state for 2D rendering using SDL3 GPU.
///
/// Use the shared `Draw` instance to draw shapes, sprites, and text.
/// All drawing operations are batched and rendered when ``flush()`` is called.
public final class Renderer: @unchecked Sendable {
    /// The SDL GPU device.
    public private(set) var device: OpaquePointer?

    /// The SDL window.
    public private(set) var window: OpaquePointer?

    /// Current command buffer.
    public private(set) var commandBuffer: OpaquePointer?

    /// Current render pass.
    public private(set) var renderPass: OpaquePointer?

    // MARK: - State Stacks

    private var colorStack: [Color] = [.white]
    private var layerStack: [Int] = [0]
    private var antialiasStack: [Bool] = [true]
    private var antialiasScaleStack: [Float] = [1.5]
    private var attributeStack: [Color] = [.clear]

    /// Current camera transform.
    public var camera: Matrix3x2 = .identity

    /// The shared renderer instance.
    public static let shared = Renderer()

    private init() {}

    // MARK: - Initialization

    /// Initializes the renderer with an SDL window.
    /// - Parameter window: The SDL_Window pointer.
    /// - Returns: True if initialization succeeded.
    @discardableResult
    public func initialize(window: OpaquePointer) -> Bool {
        self.window = window

        // Create GPU device
        self.device = SDL_CreateGPUDevice(
            SDL_GPU_SHADERFORMAT_SPIRV | SDL_GPU_SHADERFORMAT_DXIL | SDL_GPU_SHADERFORMAT_MSL,
            true, // debug mode
            nil
        )

        guard let device = self.device else {
            return false
        }

        // Claim the window
        guard SDL_ClaimWindowForGPUDevice(device, window) else {
            SDL_DestroyGPUDevice(device)
            self.device = nil
            return false
        }

        return true
    }

    /// Shuts down the renderer.
    public func shutdown() {
        if let device = device {
            SDL_DestroyGPUDevice(device)
        }
        device = nil
        window = nil
    }

    // MARK: - Frame Management

    /// Begins a new frame.
    /// - Returns: True if the frame can be rendered.
    public func beginFrame() -> Bool {
        guard let device = device else { return false }

        commandBuffer = SDL_AcquireGPUCommandBuffer(device)
        return commandBuffer != nil
    }

    /// Begins rendering to the swapchain.
    /// - Parameter clearColor: Optional clear color.
    /// - Returns: True if rendering began successfully.
    public func beginSwapchainRenderPass(clearColor: Color? = nil) -> Bool {
        guard let commandBuffer = commandBuffer, let window = window else { return false }

        var swapchainTexture: OpaquePointer?
        guard SDL_WaitAndAcquireGPUSwapchainTexture(commandBuffer, window, &swapchainTexture, nil, nil),
              let texture = swapchainTexture else {
            return false
        }

        var colorTarget = SDL_GPUColorTargetInfo()
        colorTarget.texture = texture
        colorTarget.load_op = clearColor != nil ? SDL_GPU_LOADOP_CLEAR : SDL_GPU_LOADOP_LOAD
        colorTarget.store_op = SDL_GPU_STOREOP_STORE

        if let clear = clearColor {
            colorTarget.clear_color = clear.sdlColor
        }

        renderPass = withUnsafePointer(to: &colorTarget) { ptr in
            SDL_BeginGPURenderPass(commandBuffer, ptr, 1, nil)
        }

        return renderPass != nil
    }

    /// Ends the current render pass.
    public func endRenderPass() {
        if let renderPass = renderPass {
            SDL_EndGPURenderPass(renderPass)
        }
        renderPass = nil
    }

    /// Ends the frame and submits to GPU.
    public func endFrame() {
        if let commandBuffer = commandBuffer {
            SDL_SubmitGPUCommandBuffer(commandBuffer)
        }
        commandBuffer = nil
    }

    // MARK: - Color State

    /// The current drawing color.
    public var color: Color {
        colorStack.last ?? .white
    }

    /// Pushes a color onto the stack.
    @discardableResult
    public func pushColor(_ color: Color) -> Color {
        colorStack.append(color)
        return color
    }

    /// Pops the color stack.
    @discardableResult
    public func popColor() -> Color {
        guard colorStack.count > 1 else { return colorStack.first ?? .white }
        return colorStack.removeLast()
    }

    // MARK: - Layer State

    /// The current drawing layer.
    public var layer: Int {
        layerStack.last ?? 0
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

    // MARK: - Antialias State

    /// Whether antialiasing is enabled.
    public var antialias: Bool {
        antialiasStack.last ?? true
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
        antialiasScaleStack.last ?? 1.5
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
        attributeStack.last ?? .clear
    }

    /// Pushes vertex attributes onto the stack.
    @discardableResult
    public func pushVertexAttributes(_ attributes: Color) -> Color {
        attributeStack.append(attributes)
        return attributes
    }

    /// Pops the vertex attributes stack.
    @discardableResult
    public func popVertexAttributes() -> Color {
        guard attributeStack.count > 1 else { return attributeStack.first ?? .clear }
        return attributeStack.removeLast()
    }

    // MARK: - Reset

    /// Resets all state to defaults.
    public func reset() {
        colorStack = [.white]
        layerStack = [0]
        antialiasStack = [true]
        antialiasScaleStack = [1.5]
        attributeStack = [.clear]
        camera = .identity
    }
}

// MARK: - Global Accessor

/// The global renderer instance.
public var Draw: Renderer { Renderer.shared }
