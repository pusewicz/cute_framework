/// Shader stage (vertex or fragment).
public enum ShaderStage: Int, Hashable, Codable, Sendable {
    /// Vertex shader stage.
    case vertex = 0

    /// Fragment (pixel) shader stage.
    case fragment = 1
}

/// Precompiled shader bytecode (SPIR-V).
public struct ShaderBytecode: Sendable {
    /// The raw bytecode data.
    public let data: Data

    /// The shader stage.
    public let stage: ShaderStage

    /// Creates shader bytecode from data.
    public init(data: Data, stage: ShaderStage) {
        self.data = data
        self.stage = stage
    }

    /// Creates shader bytecode from a file.
    public init?(contentsOfFile path: String, stage: ShaderStage) {
        guard let data = try? Data(contentsOf: URL(fileURLWithPath: path)) else {
            return nil
        }
        self.data = data
        self.stage = stage
    }
}

/// A GPU shader program.
///
/// Shaders are programs that run on the GPU. A shader consists of a vertex
/// shader (which transforms vertices) and a fragment shader (which colors pixels).
///
/// ## Topics
///
/// ### Creating Shaders
/// - ``init(vertexSource:fragmentSource:)``
/// - ``init(vertexBytecode:fragmentBytecode:)``
///
/// ### Shader Directory
/// - ``setDirectory(_:)``
/// - ``onChanged(_:)``
public final class Shader: Hashable, Sendable {
    /// Internal handle (would be CF_Shader in real implementation).
    internal let handle: UInt64

    /// Whether this shader is valid.
    public var isValid: Bool { handle != 0 }

    /// Creates a shader from GLSL source code.
    /// - Parameters:
    ///   - vertexSource: The vertex shader GLSL source.
    ///   - fragmentSource: The fragment shader GLSL source.
    public init(vertexSource: String, fragmentSource: String) {
        // In real implementation, this would compile the shaders
        self.handle = UInt64.random(in: 1...UInt64.max)
    }

    /// Creates a shader from file paths.
    /// - Parameters:
    ///   - vertexPath: Path to the vertex shader file.
    ///   - fragmentPath: Path to the fragment shader file.
    public convenience init?(vertexPath: String, fragmentPath: String) {
        guard let vertexSource = try? String(contentsOfFile: vertexPath),
              let fragmentSource = try? String(contentsOfFile: fragmentPath) else {
            return nil
        }
        self.init(vertexSource: vertexSource, fragmentSource: fragmentSource)
    }

    /// Creates a shader from precompiled bytecode.
    /// - Parameters:
    ///   - vertexBytecode: The compiled vertex shader.
    ///   - fragmentBytecode: The compiled fragment shader.
    public init(vertexBytecode: ShaderBytecode, fragmentBytecode: ShaderBytecode) {
        // In real implementation, this would load the bytecode
        self.handle = UInt64.random(in: 1...UInt64.max)
    }

    /// Sets the shader directory for loading shaders.
    ///
    /// Shaders in this directory can include each other with `#include`.
    public static func setDirectory(_ path: String) {
        // In real implementation, this would call cf_shader_directory
    }

    /// Registers a callback for when shaders change on disk.
    ///
    /// Useful for hot-reloading shaders during development.
    public static func onChanged(_ callback: @escaping (String) -> Void) {
        // In real implementation, this would call cf_shader_on_changed
    }

    /// Compiles GLSL source to SPIR-V bytecode.
    public static func compile(source: String, stage: ShaderStage) -> ShaderBytecode? {
        // In real implementation, this would call cf_compile_shader_to_bytecode
        return ShaderBytecode(data: Data(), stage: stage)
    }

    // MARK: - Hashable

    public static func == (lhs: Shader, rhs: Shader) -> Bool {
        return lhs.handle == rhs.handle
    }

    public func hash(into hasher: inout Hasher) {
        hasher.combine(handle)
    }
}
