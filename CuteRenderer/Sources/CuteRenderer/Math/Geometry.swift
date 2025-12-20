/// An axis-aligned bounding box (AABB).
///
/// AABBs are defined by their minimum and maximum corners. They cannot rotate.
public struct AABB: Hashable, Codable, Sendable {
    /// The minimum corner (bottom-left in standard coordinates).
    public var min: Vector2

    /// The maximum corner (top-right in standard coordinates).
    public var max: Vector2

    /// Creates an AABB from min and max corners.
    @inlinable
    public init(min: Vector2, max: Vector2) {
        self.min = min
        self.max = max
    }

    /// Creates an AABB from center point and half-extents.
    @inlinable
    public init(center: Vector2, halfExtents: Vector2) {
        self.min = center - halfExtents
        self.max = center + halfExtents
    }

    /// Creates an AABB from position (center) and size.
    @inlinable
    public init(center: Vector2, size: Vector2) {
        let halfSize = size * 0.5
        self.min = center - halfSize
        self.max = center + halfSize
    }

    /// An empty AABB at the origin.
    public static let zero = AABB(min: .zero, max: .zero)

    /// The center of the AABB.
    @inlinable
    public var center: Vector2 {
        return (min + max) * 0.5
    }

    /// The size (width and height) of the AABB.
    @inlinable
    public var size: Vector2 {
        return max - min
    }

    /// The half-extents of the AABB.
    @inlinable
    public var halfExtents: Vector2 {
        return size * 0.5
    }

    /// The width of the AABB.
    @inlinable
    public var width: Float {
        return max.x - min.x
    }

    /// The height of the AABB.
    @inlinable
    public var height: Float {
        return max.y - min.y
    }

    /// The top-left corner.
    @inlinable
    public var topLeft: Vector2 {
        return Vector2(x: min.x, y: max.y)
    }

    /// The top-right corner.
    @inlinable
    public var topRight: Vector2 {
        return max
    }

    /// The bottom-left corner.
    @inlinable
    public var bottomLeft: Vector2 {
        return min
    }

    /// The bottom-right corner.
    @inlinable
    public var bottomRight: Vector2 {
        return Vector2(x: max.x, y: min.y)
    }

    /// Returns true if this AABB contains the given point.
    @inlinable
    public func contains(_ point: Vector2) -> Bool {
        return point.x >= min.x && point.x <= max.x &&
               point.y >= min.y && point.y <= max.y
    }

    /// Returns true if this AABB overlaps with another AABB.
    @inlinable
    public func overlaps(_ other: AABB) -> Bool {
        return min.x <= other.max.x && max.x >= other.min.x &&
               min.y <= other.max.y && max.y >= other.min.y
    }

    /// Returns an AABB that contains both this and another AABB.
    @inlinable
    public func union(_ other: AABB) -> AABB {
        return AABB(
            min: Vector2(x: Swift.min(min.x, other.min.x), y: Swift.min(min.y, other.min.y)),
            max: Vector2(x: Swift.max(max.x, other.max.x), y: Swift.max(max.y, other.max.y))
        )
    }

    /// Returns the intersection of this AABB with another, or nil if they don't overlap.
    @inlinable
    public func intersection(_ other: AABB) -> AABB? {
        let newMin = Vector2(x: Swift.max(min.x, other.min.x), y: Swift.max(min.y, other.min.y))
        let newMax = Vector2(x: Swift.min(max.x, other.max.x), y: Swift.min(max.y, other.max.y))
        guard newMin.x <= newMax.x && newMin.y <= newMax.y else { return nil }
        return AABB(min: newMin, max: newMax)
    }

    /// Returns an expanded AABB by the given amount on all sides.
    @inlinable
    public func expanded(by amount: Float) -> AABB {
        return AABB(
            min: min - Vector2(amount),
            max: max + Vector2(amount)
        )
    }
}

// MARK: - Circle

/// A circle defined by center point and radius.
public struct Circle: Hashable, Codable, Sendable {
    /// The center position.
    public var position: Vector2

    /// The radius.
    public var radius: Float

    /// Creates a circle with the specified center and radius.
    @inlinable
    public init(position: Vector2, radius: Float) {
        self.position = position
        self.radius = radius
    }

    /// Creates a circle at the origin with the specified radius.
    @inlinable
    public init(radius: Float) {
        self.position = .zero
        self.radius = radius
    }

    /// A unit circle at the origin.
    public static let unit = Circle(position: .zero, radius: 1)

    /// The diameter of the circle.
    @inlinable
    public var diameter: Float {
        return radius * 2
    }

    /// The bounding box of this circle.
    @inlinable
    public var bounds: AABB {
        let r = Vector2(radius)
        return AABB(min: position - r, max: position + r)
    }

    /// Returns true if this circle contains the given point.
    @inlinable
    public func contains(_ point: Vector2) -> Bool {
        return (point - position).lengthSquared <= radius * radius
    }

    /// Returns true if this circle overlaps with another circle.
    @inlinable
    public func overlaps(_ other: Circle) -> Bool {
        let combinedRadius = radius + other.radius
        return (position - other.position).lengthSquared <= combinedRadius * combinedRadius
    }
}

// MARK: - Capsule

/// A capsule shape (two circles connected by a rectangle).
///
/// A capsule is defined by two points (the centers of the end caps) and a radius.
public struct Capsule: Hashable, Codable, Sendable {
    /// The center of one end cap.
    public var a: Vector2

    /// The center of the other end cap.
    public var b: Vector2

    /// The radius of the capsule.
    public var radius: Float

    /// Creates a capsule with the specified endpoints and radius.
    @inlinable
    public init(a: Vector2, b: Vector2, radius: Float) {
        self.a = a
        self.b = b
        self.radius = radius
    }

    /// The center of the capsule.
    @inlinable
    public var center: Vector2 {
        return (a + b) * 0.5
    }

    /// The length of the capsule's core segment.
    @inlinable
    public var length: Float {
        return (b - a).length
    }

    /// The direction from a to b (normalized).
    @inlinable
    public var direction: Vector2 {
        return (b - a).normalized()
    }

    /// The bounding box of this capsule.
    @inlinable
    public var bounds: AABB {
        let r = Vector2(radius)
        return AABB(
            min: Vector2(x: Swift.min(a.x, b.x) - radius, y: Swift.min(a.y, b.y) - radius),
            max: Vector2(x: Swift.max(a.x, b.x) + radius, y: Swift.max(a.y, b.y) + radius)
        )
    }
}

// MARK: - Rect

/// An integer rectangle, useful for pixel coordinates and viewports.
public struct Rect: Hashable, Codable, Sendable {
    /// The x position.
    public var x: Int

    /// The y position.
    public var y: Int

    /// The width.
    public var width: Int

    /// The height.
    public var height: Int

    /// Creates a rectangle with the specified position and size.
    @inlinable
    public init(x: Int, y: Int, width: Int, height: Int) {
        self.x = x
        self.y = y
        self.width = width
        self.height = height
    }

    /// Creates a rectangle from origin and size.
    @inlinable
    public init(origin: (Int, Int), size: (Int, Int)) {
        self.x = origin.0
        self.y = origin.1
        self.width = size.0
        self.height = size.1
    }

    /// A zero-sized rectangle at the origin.
    public static let zero = Rect(x: 0, y: 0, width: 0, height: 0)

    /// The origin point.
    @inlinable
    public var origin: (x: Int, y: Int) {
        return (x, y)
    }

    /// The size.
    @inlinable
    public var size: (width: Int, height: Int) {
        return (width, height)
    }

    /// The maximum x coordinate.
    @inlinable
    public var maxX: Int {
        return x + width
    }

    /// The maximum y coordinate.
    @inlinable
    public var maxY: Int {
        return y + height
    }
}

// MARK: - Ray

/// A ray defined by an origin point, direction, and maximum distance.
public struct Ray: Hashable, Codable, Sendable {
    /// The starting position of the ray.
    public var origin: Vector2

    /// The direction of the ray (should be normalized).
    public var direction: Vector2

    /// The maximum distance the ray can travel.
    public var maxDistance: Float

    /// Creates a ray with the specified origin, direction, and maximum distance.
    @inlinable
    public init(origin: Vector2, direction: Vector2, maxDistance: Float = .infinity) {
        self.origin = origin
        self.direction = direction.normalized()
        self.maxDistance = maxDistance
    }

    /// Returns the point at the given distance along the ray.
    @inlinable
    public func point(at distance: Float) -> Vector2 {
        return origin + direction * distance
    }

    /// The endpoint of the ray (at max distance).
    @inlinable
    public var endpoint: Vector2 {
        return point(at: maxDistance)
    }
}

// MARK: - Raycast Result

/// The result of a raycast operation.
public struct RaycastHit: Hashable, Codable, Sendable {
    /// Whether the ray hit something.
    public var hit: Bool

    /// The distance along the ray where the hit occurred.
    public var distance: Float

    /// The normal of the surface at the hit point.
    public var normal: Vector2

    /// Creates a raycast hit result.
    @inlinable
    public init(hit: Bool, distance: Float = 0, normal: Vector2 = .zero) {
        self.hit = hit
        self.distance = distance
        self.normal = normal
    }

    /// A miss result.
    public static let miss = RaycastHit(hit: false)
}
