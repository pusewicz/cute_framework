import Foundation

/// A rotation represented as a precomputed sine/cosine pair.
///
/// This is more efficient than storing an angle when rotating many vectors,
/// as the trigonometric functions are computed only once.
public struct SinCos: Hashable, Codable, Sendable {
    /// The sine of the angle.
    public var sin: Float

    /// The cosine of the angle.
    public var cos: Float

    /// Creates a SinCos from precomputed sine and cosine values.
    @inlinable
    public init(sin: Float, cos: Float) {
        self.sin = sin
        self.cos = cos
    }

    /// Creates a SinCos from an angle in radians.
    /// - Parameter radians: The angle in radians.
    @inlinable
    public init(radians: Float) {
        self.sin = Foundation.sin(radians)
        self.cos = Foundation.cos(radians)
    }

    /// Creates a SinCos from an angle in degrees.
    /// - Parameter degrees: The angle in degrees.
    @inlinable
    public init(degrees: Float) {
        let radians = degrees * .pi / 180.0
        self.sin = Foundation.sin(radians)
        self.cos = Foundation.cos(radians)
    }

    /// Identity rotation (no rotation).
    public static let identity = SinCos(sin: 0, cos: 1)

    /// The angle in radians.
    @inlinable
    public var radians: Float {
        return atan2(sin, cos)
    }

    /// The angle in degrees.
    @inlinable
    public var degrees: Float {
        return radians * 180.0 / .pi
    }

    /// Returns the inverse (negative) rotation.
    @inlinable
    public var inverse: SinCos {
        return SinCos(sin: -sin, cos: cos)
    }

    /// Combines two rotations.
    @inlinable
    public func combined(with other: SinCos) -> SinCos {
        return SinCos(
            sin: sin * other.cos + cos * other.sin,
            cos: cos * other.cos - sin * other.sin
        )
    }

    /// Returns the x-axis (right direction) for this rotation.
    @inlinable
    public var xAxis: Vector2 {
        return Vector2(x: cos, y: sin)
    }

    /// Returns the y-axis (up direction) for this rotation.
    @inlinable
    public var yAxis: Vector2 {
        return Vector2(x: -sin, y: cos)
    }
}

// MARK: - Operators

extension SinCos {
    /// Rotates a vector by this rotation.
    @inlinable
    public static func * (rotation: SinCos, vector: Vector2) -> Vector2 {
        return vector.rotated(by: rotation)
    }

    /// Combines two rotations.
    @inlinable
    public static func * (lhs: SinCos, rhs: SinCos) -> SinCos {
        return lhs.combined(with: rhs)
    }
}
