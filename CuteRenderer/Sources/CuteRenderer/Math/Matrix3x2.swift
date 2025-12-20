/// A 3x2 affine transformation matrix for 2D graphics.
///
/// This matrix supports translation, rotation, and scale transformations.
/// It's stored as a 2x2 rotation/scale matrix plus a translation vector.
///
/// The matrix is laid out as:
/// ```
/// | m.x.x  m.y.x  p.x |
/// | m.x.y  m.y.y  p.y |
/// |   0      0     1  |
/// ```
public struct Matrix3x2: Hashable, Codable, Sendable {
    /// The x column of the 2x2 rotation/scale portion.
    public var x: Vector2

    /// The y column of the 2x2 rotation/scale portion.
    public var y: Vector2

    /// The translation (position) component.
    public var position: Vector2

    /// Creates a matrix from column vectors.
    @inlinable
    public init(x: Vector2, y: Vector2, position: Vector2) {
        self.x = x
        self.y = y
        self.position = position
    }

    /// The identity matrix (no transformation).
    public static let identity = Matrix3x2(
        x: Vector2(x: 1, y: 0),
        y: Vector2(x: 0, y: 1),
        position: .zero
    )

    /// Creates a translation matrix.
    @inlinable
    public static func translation(_ translation: Vector2) -> Matrix3x2 {
        return Matrix3x2(
            x: Vector2(x: 1, y: 0),
            y: Vector2(x: 0, y: 1),
            position: translation
        )
    }

    /// Creates a translation matrix.
    @inlinable
    public static func translation(x: Float, y: Float) -> Matrix3x2 {
        return translation(Vector2(x: x, y: y))
    }

    /// Creates a uniform scale matrix.
    @inlinable
    public static func scale(_ scale: Float) -> Matrix3x2 {
        return Matrix3x2(
            x: Vector2(x: scale, y: 0),
            y: Vector2(x: 0, y: scale),
            position: .zero
        )
    }

    /// Creates a non-uniform scale matrix.
    @inlinable
    public static func scale(_ scale: Vector2) -> Matrix3x2 {
        return Matrix3x2(
            x: Vector2(x: scale.x, y: 0),
            y: Vector2(x: 0, y: scale.y),
            position: .zero
        )
    }

    /// Creates a non-uniform scale matrix.
    @inlinable
    public static func scale(x: Float, y: Float) -> Matrix3x2 {
        return scale(Vector2(x: x, y: y))
    }

    /// Creates a rotation matrix from an angle in radians.
    @inlinable
    public static func rotation(radians: Float) -> Matrix3x2 {
        let c = cos(radians)
        let s = sin(radians)
        return Matrix3x2(
            x: Vector2(x: c, y: s),
            y: Vector2(x: -s, y: c),
            position: .zero
        )
    }

    /// Creates a rotation matrix from an angle in degrees.
    @inlinable
    public static func rotation(degrees: Float) -> Matrix3x2 {
        return rotation(radians: degrees * .pi / 180.0)
    }

    /// Creates a rotation matrix from a SinCos pair.
    @inlinable
    public static func rotation(_ sinCos: SinCos) -> Matrix3x2 {
        return Matrix3x2(
            x: Vector2(x: sinCos.cos, y: sinCos.sin),
            y: Vector2(x: -sinCos.sin, y: sinCos.cos),
            position: .zero
        )
    }

    /// Creates a transformation matrix with translation, scale, and rotation.
    @inlinable
    public static func transform(
        position: Vector2 = .zero,
        scale: Vector2 = .one,
        rotation: SinCos = .identity
    ) -> Matrix3x2 {
        return Matrix3x2(
            x: Vector2(x: rotation.cos * scale.x, y: rotation.sin * scale.x),
            y: Vector2(x: -rotation.sin * scale.y, y: rotation.cos * scale.y),
            position: position
        )
    }

    /// Transforms a point by this matrix.
    @inlinable
    public func transformPoint(_ point: Vector2) -> Vector2 {
        return Vector2(
            x: x.x * point.x + y.x * point.y + position.x,
            y: x.y * point.x + y.y * point.y + position.y
        )
    }

    /// Transforms a direction (ignoring translation).
    @inlinable
    public func transformDirection(_ direction: Vector2) -> Vector2 {
        return Vector2(
            x: x.x * direction.x + y.x * direction.y,
            y: x.y * direction.x + y.y * direction.y
        )
    }

    /// Multiplies this matrix by another.
    @inlinable
    public func multiplied(by other: Matrix3x2) -> Matrix3x2 {
        return Matrix3x2(
            x: Vector2(
                x: x.x * other.x.x + y.x * other.x.y,
                y: x.y * other.x.x + y.y * other.x.y
            ),
            y: Vector2(
                x: x.x * other.y.x + y.x * other.y.y,
                y: x.y * other.y.x + y.y * other.y.y
            ),
            position: Vector2(
                x: x.x * other.position.x + y.x * other.position.y + position.x,
                y: x.y * other.position.x + y.y * other.position.y + position.y
            )
        )
    }

    /// Returns the inverse of this matrix.
    ///
    /// - Warning: If the matrix is not invertible (determinant is zero), returns the identity matrix.
    @inlinable
    public var inverse: Matrix3x2 {
        let det = x.x * y.y - x.y * y.x
        guard abs(det) > 1e-10 else { return .identity }

        let invDet = 1.0 / det
        let invX = Vector2(x: y.y * invDet, y: -x.y * invDet)
        let invY = Vector2(x: -y.x * invDet, y: x.x * invDet)
        let invP = Vector2(
            x: -(invX.x * position.x + invY.x * position.y),
            y: -(invX.y * position.x + invY.y * position.y)
        )

        return Matrix3x2(x: invX, y: invY, position: invP)
    }
}

// MARK: - Operators

extension Matrix3x2 {
    /// Transforms a point by this matrix.
    @inlinable
    public static func * (matrix: Matrix3x2, point: Vector2) -> Vector2 {
        return matrix.transformPoint(point)
    }

    /// Multiplies two matrices.
    @inlinable
    public static func * (lhs: Matrix3x2, rhs: Matrix3x2) -> Matrix3x2 {
        return lhs.multiplied(by: rhs)
    }
}
