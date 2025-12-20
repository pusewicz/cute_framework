/// Vertex attribute format.
public enum VertexFormat: Int, Hashable, Codable, Sendable {
    /// 32-bit float.
    case float = 0

    /// 2x 32-bit floats.
    case float2 = 1

    /// 3x 32-bit floats.
    case float3 = 2

    /// 4x 32-bit floats.
    case float4 = 3

    /// 8-bit unsigned int.
    case ubyte = 4

    /// 2x 8-bit unsigned ints.
    case ubyte2 = 5

    /// 4x 8-bit unsigned ints.
    case ubyte4 = 6

    /// 4x 8-bit unsigned ints, normalized to 0-1.
    case ubyte4Norm = 7

    /// 16-bit signed int.
    case short = 8

    /// 2x 16-bit signed ints.
    case short2 = 9

    /// 4x 16-bit signed ints.
    case short4 = 10

    /// 16-bit unsigned int.
    case ushort = 11

    /// 2x 16-bit unsigned ints.
    case ushort2 = 12

    /// 4x 16-bit unsigned ints.
    case ushort4 = 13

    /// 32-bit signed int.
    case int = 14

    /// 2x 32-bit signed ints.
    case int2 = 15

    /// 3x 32-bit signed ints.
    case int3 = 16

    /// 4x 32-bit signed ints.
    case int4 = 17

    /// Size in bytes.
    public var byteSize: Int {
        switch self {
        case .float: return 4
        case .float2: return 8
        case .float3: return 12
        case .float4: return 16
        case .ubyte: return 1
        case .ubyte2: return 2
        case .ubyte4, .ubyte4Norm: return 4
        case .short, .ushort: return 2
        case .short2, .ushort2: return 4
        case .short4, .ushort4: return 8
        case .int: return 4
        case .int2: return 8
        case .int3: return 12
        case .int4: return 16
        }
    }
}

/// Describes a vertex attribute layout.
public struct VertexAttribute: Hashable, Sendable {
    /// The attribute name (for debugging).
    public var name: String

    /// The format of the attribute.
    public var format: VertexFormat

    /// The offset in bytes from the start of the vertex.
    public var offset: Int

    /// Creates a vertex attribute.
    public init(name: String, format: VertexFormat, offset: Int) {
        self.name = name
        self.format = format
        self.offset = offset
    }
}

/// Index format for mesh indices.
public enum IndexFormat: Int, Hashable, Codable, Sendable {
    /// 16-bit unsigned indices.
    case uint16 = 0

    /// 32-bit unsigned indices.
    case uint32 = 1
}

/// Primitive topology for rendering.
public enum PrimitiveType: Int, Hashable, Codable, Sendable {
    /// Render as separate triangles.
    case triangles = 0

    /// Render as a triangle strip.
    case triangleStrip = 1

    /// Render as separate lines.
    case lines = 2

    /// Render as a line strip.
    case lineStrip = 3

    /// Render as points.
    case points = 4
}

/// A mesh containing vertex and index data.
///
/// Meshes hold geometry data that can be rendered to the screen.
/// They consist of vertices (with attributes like position, UV, color)
/// and optional indices for indexed rendering.
///
/// ## Topics
///
/// ### Creating Meshes
/// - ``init(attributes:stride:usage:)``
///
/// ### Updating Data
/// - ``updateVertices(_:count:)``
/// - ``updateIndices(_:count:format:)``
///
/// ### Rendering
/// - ``drawElements(count:offset:)``
/// - ``drawArrays(count:offset:)``
public final class Mesh: Hashable, @unchecked Sendable {
    /// Internal handle (would be CF_Mesh in real implementation).
    internal let handle: UInt64

    /// Vertex attributes.
    public let attributes: [VertexAttribute]

    /// Stride in bytes between vertices.
    public let stride: Int

    /// Number of vertices.
    public private(set) var vertexCount: Int = 0

    /// Number of indices.
    public private(set) var indexCount: Int = 0

    /// Index format.
    public private(set) var indexFormat: IndexFormat = .uint16

    /// Creates a mesh with the specified vertex layout.
    public init(
        attributes: [VertexAttribute],
        stride: Int,
        usage: BufferUsage = .dynamic
    ) {
        self.attributes = attributes
        self.stride = stride
        self.handle = UInt64.random(in: 1...UInt64.max)
    }

    /// Updates vertex data.
    public func updateVertices(_ data: UnsafeRawPointer, count: Int) {
        self.vertexCount = count
        // In real implementation, this would call cf_mesh_update_vertex_data
    }

    /// Updates vertex data from an array.
    public func updateVertices<T>(_ vertices: [T]) {
        vertices.withUnsafeBytes { buffer in
            updateVertices(buffer.baseAddress!, count: vertices.count)
        }
    }

    /// Updates index data.
    public func updateIndices(_ data: UnsafeRawPointer, count: Int, format: IndexFormat = .uint16) {
        self.indexCount = count
        self.indexFormat = format
        // In real implementation, this would call cf_mesh_update_index_data
    }

    /// Updates index data from an array.
    public func updateIndices(_ indices: [UInt16]) {
        indices.withUnsafeBytes { buffer in
            updateIndices(buffer.baseAddress!, count: indices.count, format: .uint16)
        }
    }

    /// Updates index data from an array.
    public func updateIndices(_ indices: [UInt32]) {
        indices.withUnsafeBytes { buffer in
            updateIndices(buffer.baseAddress!, count: indices.count, format: .uint32)
        }
    }

    // MARK: - Hashable

    public static func == (lhs: Mesh, rhs: Mesh) -> Bool {
        return lhs.handle == rhs.handle
    }

    public func hash(into hasher: inout Hasher) {
        hasher.combine(handle)
    }
}

/// Buffer usage hints.
public enum BufferUsage: Int, Hashable, Codable, Sendable {
    /// Data is set once and rarely changed.
    case `static` = 0

    /// Data is changed frequently.
    case dynamic = 1

    /// Data is changed every frame.
    case stream = 2
}
