/// A 2D vector with x and y components.
///
/// `Vector2` is the fundamental 2D math type used throughout CuteRenderer for positions,
/// directions, scales, and UV coordinates.
///
/// ## Topics
///
/// ### Creating Vectors
/// - ``init(x:y:)``
/// - ``init(_:)``
/// - ``zero``
/// - ``one``
///
/// ### Properties
/// - ``x``
/// - ``y``
/// - ``length``
/// - ``lengthSquared``
///
/// ### Operations
/// - ``normalized()``
/// - ``dot(_:)``
/// - ``cross(_:)``
/// - ``perpendicular``
/// - ``rotated(by:)``
public struct Vector2: Hashable, Codable, Sendable {
    /// The x component.
    public var x: Float

    /// The y component.
    public var y: Float

    /// Creates a vector with the specified x and y values.
    /// - Parameters:
    ///   - x: The x component.
    ///   - y: The y component.
    @inlinable
    public init(x: Float, y: Float) {
        self.x = x
        self.y = y
    }

    /// Creates a vector with both components set to the same value.
    /// - Parameter value: The value for both x and y components.
    @inlinable
    public init(_ value: Float) {
        self.x = value
        self.y = value
    }

    /// A vector with both components set to zero.
    public static let zero = Vector2(x: 0, y: 0)

    /// A vector with both components set to one.
    public static let one = Vector2(x: 1, y: 1)

    /// A unit vector pointing right (positive x).
    public static let right = Vector2(x: 1, y: 0)

    /// A unit vector pointing left (negative x).
    public static let left = Vector2(x: -1, y: 0)

    /// A unit vector pointing up (positive y).
    public static let up = Vector2(x: 0, y: 1)

    /// A unit vector pointing down (negative y).
    public static let down = Vector2(x: 0, y: -1)

    /// The length (magnitude) of the vector.
    @inlinable
    public var length: Float {
        return (x * x + y * y).squareRoot()
    }

    /// The squared length of the vector.
    ///
    /// Prefer this over `length` when comparing distances, as it avoids the square root calculation.
    @inlinable
    public var lengthSquared: Float {
        return x * x + y * y
    }

    /// Returns a normalized (unit length) version of this vector.
    ///
    /// If the vector has zero length, returns a zero vector.
    @inlinable
    public func normalized() -> Vector2 {
        let len = length
        guard len > 0 else { return .zero }
        return Vector2(x: x / len, y: y / len)
    }

    /// Computes the dot product with another vector.
    /// - Parameter other: The other vector.
    /// - Returns: The dot product.
    @inlinable
    public func dot(_ other: Vector2) -> Float {
        return x * other.x + y * other.y
    }

    /// Computes the 2D cross product (perpendicular dot product) with another vector.
    /// - Parameter other: The other vector.
    /// - Returns: The z-component of the 3D cross product.
    @inlinable
    public func cross(_ other: Vector2) -> Float {
        return x * other.y - y * other.x
    }

    /// Returns a vector perpendicular to this one (rotated 90 degrees counter-clockwise).
    @inlinable
    public var perpendicular: Vector2 {
        return Vector2(x: -y, y: x)
    }

    /// Returns this vector rotated by the given angle in radians.
    /// - Parameter angle: The rotation angle in radians.
    /// - Returns: The rotated vector.
    @inlinable
    public func rotated(by angle: Float) -> Vector2 {
        let c = cos(angle)
        let s = sin(angle)
        return Vector2(x: x * c - y * s, y: x * s + y * c)
    }

    /// Returns this vector rotated by a precomputed sin/cos pair.
    /// - Parameter rotation: The rotation as a SinCos value.
    /// - Returns: The rotated vector.
    @inlinable
    public func rotated(by rotation: SinCos) -> Vector2 {
        return Vector2(x: x * rotation.cos - y * rotation.sin,
                       y: x * rotation.sin + y * rotation.cos)
    }

    /// Linearly interpolates between this vector and another.
    /// - Parameters:
    ///   - other: The target vector.
    ///   - t: The interpolation factor (0 = self, 1 = other).
    /// - Returns: The interpolated vector.
    @inlinable
    public func lerp(to other: Vector2, t: Float) -> Vector2 {
        return self + (other - self) * t
    }

    /// Returns the distance to another vector.
    /// - Parameter other: The other vector.
    /// - Returns: The distance between the two vectors.
    @inlinable
    public func distance(to other: Vector2) -> Float {
        return (self - other).length
    }

    /// Returns the squared distance to another vector.
    /// - Parameter other: The other vector.
    /// - Returns: The squared distance between the two vectors.
    @inlinable
    public func distanceSquared(to other: Vector2) -> Float {
        return (self - other).lengthSquared
    }
}

// MARK: - Operators

extension Vector2 {
    @inlinable
    public static func + (lhs: Vector2, rhs: Vector2) -> Vector2 {
        return Vector2(x: lhs.x + rhs.x, y: lhs.y + rhs.y)
    }

    @inlinable
    public static func - (lhs: Vector2, rhs: Vector2) -> Vector2 {
        return Vector2(x: lhs.x - rhs.x, y: lhs.y - rhs.y)
    }

    @inlinable
    public static func * (lhs: Vector2, rhs: Vector2) -> Vector2 {
        return Vector2(x: lhs.x * rhs.x, y: lhs.y * rhs.y)
    }

    @inlinable
    public static func / (lhs: Vector2, rhs: Vector2) -> Vector2 {
        return Vector2(x: lhs.x / rhs.x, y: lhs.y / rhs.y)
    }

    @inlinable
    public static func * (lhs: Vector2, rhs: Float) -> Vector2 {
        return Vector2(x: lhs.x * rhs, y: lhs.y * rhs)
    }

    @inlinable
    public static func * (lhs: Float, rhs: Vector2) -> Vector2 {
        return Vector2(x: lhs * rhs.x, y: lhs * rhs.y)
    }

    @inlinable
    public static func / (lhs: Vector2, rhs: Float) -> Vector2 {
        return Vector2(x: lhs.x / rhs, y: lhs.y / rhs)
    }

    @inlinable
    public static prefix func - (v: Vector2) -> Vector2 {
        return Vector2(x: -v.x, y: -v.y)
    }

    @inlinable
    public static func += (lhs: inout Vector2, rhs: Vector2) {
        lhs = lhs + rhs
    }

    @inlinable
    public static func -= (lhs: inout Vector2, rhs: Vector2) {
        lhs = lhs - rhs
    }

    @inlinable
    public static func *= (lhs: inout Vector2, rhs: Float) {
        lhs = lhs * rhs
    }

    @inlinable
    public static func /= (lhs: inout Vector2, rhs: Float) {
        lhs = lhs / rhs
    }
}

// MARK: - CustomStringConvertible

extension Vector2: CustomStringConvertible {
    public var description: String {
        return "(\(x), \(y))"
    }
}

// MARK: - ExpressibleByArrayLiteral

extension Vector2: ExpressibleByArrayLiteral {
    public init(arrayLiteral elements: Float...) {
        precondition(elements.count == 2, "Vector2 requires exactly 2 elements")
        self.x = elements[0]
        self.y = elements[1]
    }
}
