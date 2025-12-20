/// Blend operation for alpha blending.
public enum BlendOp: Int, Hashable, Codable, Sendable {
    /// Result = Source + Destination.
    case add = 0

    /// Result = Source - Destination.
    case subtract = 1

    /// Result = Destination - Source.
    case reverseSubtract = 2

    /// Result = min(Source, Destination).
    case min = 3

    /// Result = max(Source, Destination).
    case max = 4
}

/// Blend factor for alpha blending equations.
public enum BlendFactor: Int, Hashable, Codable, Sendable {
    /// Factor = 0.
    case zero = 0

    /// Factor = 1.
    case one = 1

    /// Factor = Source Color.
    case srcColor = 2

    /// Factor = 1 - Source Color.
    case oneMinusSrcColor = 3

    /// Factor = Dest Color.
    case dstColor = 4

    /// Factor = 1 - Dest Color.
    case oneMinusDstColor = 5

    /// Factor = Source Alpha.
    case srcAlpha = 6

    /// Factor = 1 - Source Alpha.
    case oneMinusSrcAlpha = 7

    /// Factor = Dest Alpha.
    case dstAlpha = 8

    /// Factor = 1 - Dest Alpha.
    case oneMinusDstAlpha = 9

    /// Factor = Constant Color.
    case constantColor = 10

    /// Factor = 1 - Constant Color.
    case oneMinusConstantColor = 11

    /// Factor = min(Source Alpha, 1 - Dest Alpha).
    case srcAlphaSaturate = 12
}

/// Compare function for depth/stencil testing.
public enum CompareFunction: Int, Hashable, Codable, Sendable {
    /// Always fail.
    case never = 0

    /// Pass if less than.
    case less = 1

    /// Pass if equal.
    case equal = 2

    /// Pass if less than or equal.
    case lessEqual = 3

    /// Pass if greater than.
    case greater = 4

    /// Pass if not equal.
    case notEqual = 5

    /// Pass if greater than or equal.
    case greaterEqual = 6

    /// Always pass.
    case always = 7
}

/// Face culling mode.
public enum CullMode: Int, Hashable, Codable, Sendable {
    /// No culling.
    case none = 0

    /// Cull front faces.
    case front = 1

    /// Cull back faces.
    case back = 2
}

/// Blend state configuration.
public struct BlendState: Hashable, Codable, Sendable {
    /// Whether blending is enabled.
    public var enabled: Bool

    /// Source color blend factor.
    public var srcColorFactor: BlendFactor

    /// Destination color blend factor.
    public var dstColorFactor: BlendFactor

    /// Color blend operation.
    public var colorOp: BlendOp

    /// Source alpha blend factor.
    public var srcAlphaFactor: BlendFactor

    /// Destination alpha blend factor.
    public var dstAlphaFactor: BlendFactor

    /// Alpha blend operation.
    public var alphaOp: BlendOp

    /// Creates a blend state.
    public init(
        enabled: Bool = true,
        srcColorFactor: BlendFactor = .srcAlpha,
        dstColorFactor: BlendFactor = .oneMinusSrcAlpha,
        colorOp: BlendOp = .add,
        srcAlphaFactor: BlendFactor = .one,
        dstAlphaFactor: BlendFactor = .oneMinusSrcAlpha,
        alphaOp: BlendOp = .add
    ) {
        self.enabled = enabled
        self.srcColorFactor = srcColorFactor
        self.dstColorFactor = dstColorFactor
        self.colorOp = colorOp
        self.srcAlphaFactor = srcAlphaFactor
        self.dstAlphaFactor = dstAlphaFactor
        self.alphaOp = alphaOp
    }

    /// No blending (opaque).
    public static let opaque = BlendState(enabled: false)

    /// Standard alpha blending.
    public static let alpha = BlendState()

    /// Premultiplied alpha blending.
    public static let premultiplied = BlendState(
        srcColorFactor: .one,
        dstColorFactor: .oneMinusSrcAlpha
    )

    /// Additive blending.
    public static let additive = BlendState(
        srcColorFactor: .srcAlpha,
        dstColorFactor: .one
    )

    /// Multiply blending.
    public static let multiply = BlendState(
        srcColorFactor: .dstColor,
        dstColorFactor: .zero
    )
}

/// Render state configuration.
public struct RenderState: Hashable, Codable, Sendable {
    /// Blend state.
    public var blend: BlendState

    /// Depth compare function.
    public var depthCompare: CompareFunction

    /// Whether to write to depth buffer.
    public var depthWrite: Bool

    /// Cull mode.
    public var cull: CullMode

    /// Creates a render state.
    public init(
        blend: BlendState = .alpha,
        depthCompare: CompareFunction = .always,
        depthWrite: Bool = false,
        cull: CullMode = .none
    ) {
        self.blend = blend
        self.depthCompare = depthCompare
        self.depthWrite = depthWrite
        self.cull = cull
    }

    /// Default 2D render state (alpha blending, no depth).
    public static let default2D = RenderState()

    /// 3D render state with depth testing.
    public static let default3D = RenderState(
        blend: .opaque,
        depthCompare: .less,
        depthWrite: true,
        cull: .back
    )
}

/// A material holds shader uniforms and textures.
///
/// Materials store the inputs to shaders: uniform values and texture bindings.
/// They also contain render state like blend mode and depth testing.
///
/// ## Topics
///
/// ### Creating Materials
/// - ``init()``
///
/// ### Setting Uniforms
/// - ``setUniform(name:value:stage:)``
/// - ``setTexture(name:texture:stage:)``
///
/// ### Render State
/// - ``renderState``
public final class Material: Hashable, @unchecked Sendable {
    /// Internal handle (would be CF_Material in real implementation).
    internal let handle: UInt64

    /// The render state.
    public var renderState: RenderState = .default2D

    /// Uniform values (name -> data).
    private var uniforms: [String: UniformValue] = [:]

    /// Texture bindings.
    private var textures: [String: Texture] = [:]

    /// Creates an empty material.
    public init() {
        self.handle = UInt64.random(in: 1...UInt64.max)
    }

    /// Sets a float uniform.
    public func setUniform(name: String, value: Float, stage: ShaderStage = .fragment) {
        uniforms[name] = .float(value)
    }

    /// Sets a Vector2 uniform.
    public func setUniform(name: String, value: Vector2, stage: ShaderStage = .fragment) {
        uniforms[name] = .vec2(value)
    }

    /// Sets a Color uniform.
    public func setUniform(name: String, value: Color, stage: ShaderStage = .fragment) {
        uniforms[name] = .color(value)
    }

    /// Sets a Matrix3x2 uniform.
    public func setUniform(name: String, value: Matrix3x2, stage: ShaderStage = .fragment) {
        uniforms[name] = .matrix(value)
    }

    /// Sets a texture binding.
    public func setTexture(name: String, texture: Texture, stage: ShaderStage = .fragment) {
        textures[name] = texture
    }

    // MARK: - Hashable

    public static func == (lhs: Material, rhs: Material) -> Bool {
        return lhs.handle == rhs.handle
    }

    public func hash(into hasher: inout Hasher) {
        hasher.combine(handle)
    }
}

/// Internal uniform value storage.
private enum UniformValue: Sendable {
    case float(Float)
    case vec2(Vector2)
    case color(Color)
    case matrix(Matrix3x2)
}
