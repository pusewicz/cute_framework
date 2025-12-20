import CSDL3

// MARK: - Shape Drawing Extension

extension Renderer {
    // MARK: - Quad / Box Drawing

    /// Draws a quad wireframe from an AABB.
    public func quad(_ aabb: AABB, thickness: Float, chubbiness: Float = 0) {
        quad(aabb.bottomLeft, aabb.bottomRight, aabb.topRight, aabb.topLeft,
             thickness: thickness, chubbiness: chubbiness)
    }

    /// Draws a quad wireframe from four corners.
    public func quad(_ p0: Vector2, _ p1: Vector2, _ p2: Vector2, _ p3: Vector2,
                     thickness: Float, chubbiness: Float = 0) {
        // TODO: Queue draw command for batching
    }

    /// Draws a filled quad from an AABB.
    public func quadFill(_ aabb: AABB, chubbiness: Float = 0) {
        quadFill(aabb.bottomLeft, aabb.bottomRight, aabb.topRight, aabb.topLeft,
                 chubbiness: chubbiness)
    }

    /// Draws a filled quad from four corners.
    public func quadFill(_ p0: Vector2, _ p1: Vector2, _ p2: Vector2, _ p3: Vector2,
                         chubbiness: Float = 0) {
        // TODO: Queue draw command for batching
    }

    /// Draws a box wireframe with rounded corners.
    public func boxRounded(_ aabb: AABB, thickness: Float, radius: Float) {
        quad(aabb, thickness: thickness, chubbiness: radius)
    }

    /// Draws a filled box with rounded corners.
    public func boxRoundedFill(_ aabb: AABB, radius: Float) {
        quadFill(aabb, chubbiness: radius)
    }

    // MARK: - Circle Drawing

    /// Draws a circle wireframe.
    public func circle(_ circle: Circle, thickness: Float) {
        self.circle(center: circle.position, radius: circle.radius, thickness: thickness)
    }

    /// Draws a circle wireframe.
    public func circle(center: Vector2, radius: Float, thickness: Float) {
        // TODO: Queue draw command for batching
    }

    /// Draws a filled circle.
    public func circleFill(_ circle: Circle) {
        circleFill(center: circle.position, radius: circle.radius)
    }

    /// Draws a filled circle.
    public func circleFill(center: Vector2, radius: Float) {
        // TODO: Queue draw command for batching
    }

    // MARK: - Capsule Drawing

    /// Draws a capsule wireframe.
    public func capsule(_ capsule: Capsule, thickness: Float) {
        self.capsule(from: capsule.a, to: capsule.b, radius: capsule.radius, thickness: thickness)
    }

    /// Draws a capsule wireframe.
    public func capsule(from: Vector2, to: Vector2, radius: Float, thickness: Float) {
        // TODO: Queue draw command for batching
    }

    /// Draws a filled capsule.
    public func capsuleFill(_ capsule: Capsule) {
        capsuleFill(from: capsule.a, to: capsule.b, radius: capsule.radius)
    }

    /// Draws a filled capsule.
    public func capsuleFill(from: Vector2, to: Vector2, radius: Float) {
        // TODO: Queue draw command for batching
    }

    // MARK: - Triangle Drawing

    /// Draws a triangle wireframe.
    public func triangle(_ p0: Vector2, _ p1: Vector2, _ p2: Vector2,
                         thickness: Float, chubbiness: Float = 0) {
        // TODO: Queue draw command for batching
    }

    /// Draws a filled triangle.
    public func triangleFill(_ p0: Vector2, _ p1: Vector2, _ p2: Vector2,
                             chubbiness: Float = 0) {
        // TODO: Queue draw command for batching
    }

    // MARK: - Line Drawing

    /// Draws a line.
    public func line(from: Vector2, to: Vector2, thickness: Float) {
        // TODO: Queue draw command for batching
    }

    /// Draws a polyline (connected line segments).
    public func polyline(_ points: [Vector2], thickness: Float, loop: Bool = false) {
        guard points.count >= 2 else { return }
        for i in 0..<(points.count - 1) {
            line(from: points[i], to: points[i + 1], thickness: thickness)
        }
        if loop && points.count > 2 {
            line(from: points[points.count - 1], to: points[0], thickness: thickness)
        }
    }

    /// Draws a quadratic bezier curve.
    public func bezier(start: Vector2, control: Vector2, end: Vector2,
                       segments: Int = 20, thickness: Float) {
        var points: [Vector2] = []
        for i in 0...segments {
            let t = Float(i) / Float(segments)
            let u = 1 - t
            let p = start * (u * u) + control * (2 * u * t) + end * (t * t)
            points.append(p)
        }
        polyline(points, thickness: thickness)
    }

    /// Draws a cubic bezier curve.
    public func bezier(start: Vector2, control1: Vector2, control2: Vector2, end: Vector2,
                       segments: Int = 20, thickness: Float) {
        var points: [Vector2] = []
        for i in 0...segments {
            let t = Float(i) / Float(segments)
            let u = 1 - t
            let p = start * (u * u * u) +
                    control1 * (3 * u * u * t) +
                    control2 * (3 * u * t * t) +
                    end * (t * t * t)
            points.append(p)
        }
        polyline(points, thickness: thickness)
    }

    /// Draws an arrow.
    public func arrow(from: Vector2, to: Vector2, thickness: Float, arrowWidth: Float) {
        line(from: from, to: to, thickness: thickness)

        let dir = (to - from).normalized()
        let perp = dir.perpendicular
        let arrowBase = to - dir * arrowWidth
        let p0 = arrowBase + perp * (arrowWidth * 0.5)
        let p1 = arrowBase - perp * (arrowWidth * 0.5)

        triangleFill(to, p0, p1)
    }

    // MARK: - Polygon Drawing

    /// Draws a filled polygon (max 8 vertices for SDF).
    public func polygonFill(_ points: [Vector2], chubbiness: Float = 0) {
        precondition(points.count <= 8, "Polygon fill limited to 8 vertices")
        // TODO: Queue draw command for batching
    }

    /// Draws a filled simple polygon (triangulated, no limit on vertices).
    public func polygonFillSimple(_ points: [Vector2]) {
        guard points.count >= 3 else { return }
        // Simple ear-clipping triangulation
        for i in 1..<(points.count - 1) {
            triangleFill(points[0], points[i], points[i + 1])
        }
    }
}
