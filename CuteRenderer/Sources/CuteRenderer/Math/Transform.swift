/// A 2D transformation consisting of rotation and translation.
///
/// This is primarily used for physics/collision detection where scale is not needed.
/// For graphics transformations with scale support, use ``Matrix3x2``.
public struct Transform: Hashable, Codable, Sendable {
    /// The rotation component.
    public var rotation: SinCos

    /// The position (translation) component.
    public var position: Vector2

    /// Creates a transform with the specified rotation and position.
    @inlinable
    public init(rotation: SinCos = .identity, position: Vector2 = .zero) {
        self.rotation = rotation
        self.position = position
    }

    /// Creates a transform from an angle in radians and a position.
    @inlinable
    public init(radians: Float, position: Vector2 = .zero) {
        self.rotation = SinCos(radians: radians)
        self.position = position
    }

    /// The identity transform (no rotation, at origin).
    public static let identity = Transform(rotation: .identity, position: .zero)

    /// Returns the inverse of this transform.
    @inlinable
    public var inverse: Transform {
        let invRotation = rotation.inverse
        return Transform(
            rotation: invRotation,
            position: -(invRotation * position)
        )
    }

    /// Transforms a point from local space to world space.
    @inlinable
    public func transformPoint(_ point: Vector2) -> Vector2 {
        return (rotation * point) + position
    }

    /// Transforms a point from world space to local space.
    @inlinable
    public func inverseTransformPoint(_ point: Vector2) -> Vector2 {
        return rotation.inverse * (point - position)
    }

    /// Transforms a direction (rotation only, no translation).
    @inlinable
    public func transformDirection(_ direction: Vector2) -> Vector2 {
        return rotation * direction
    }

    /// Inverse transforms a direction.
    @inlinable
    public func inverseTransformDirection(_ direction: Vector2) -> Vector2 {
        return rotation.inverse * direction
    }

    /// Combines this transform with another (applies other first, then self).
    @inlinable
    public func combined(with other: Transform) -> Transform {
        return Transform(
            rotation: rotation * other.rotation,
            position: transformPoint(other.position)
        )
    }
}

// MARK: - Operators

extension Transform {
    /// Transforms a point.
    @inlinable
    public static func * (transform: Transform, point: Vector2) -> Vector2 {
        return transform.transformPoint(point)
    }

    /// Combines two transforms.
    @inlinable
    public static func * (lhs: Transform, rhs: Transform) -> Transform {
        return lhs.combined(with: rhs)
    }
}
