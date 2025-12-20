// MARK: - Shape Drawing Extension

extension DrawContext {
    // MARK: - Quad / Box Drawing

    /// Draws a quad wireframe from an AABB.
    /// - Parameters:
    ///   - aabb: The axis-aligned bounding box.
    ///   - thickness: The line thickness.
    ///   - chubbiness: Corner rounding amount.
    public func quad(_ aabb: AABB, thickness: Float, chubbiness: Float = 0) {
        // In real implementation, this would queue a draw command
    }

    /// Draws a quad wireframe from four corners.
    /// - Parameters:
    ///   - p0: First corner (counter-clockwise).
    ///   - p1: Second corner.
    ///   - p2: Third corner.
    ///   - p3: Fourth corner.
    ///   - thickness: The line thickness.
    ///   - chubbiness: Corner rounding amount.
    public func quad(_ p0: Vector2, _ p1: Vector2, _ p2: Vector2, _ p3: Vector2,
                     thickness: Float, chubbiness: Float = 0) {
        // In real implementation, this would queue a draw command
    }

    /// Draws a filled quad from an AABB.
    /// - Parameters:
    ///   - aabb: The axis-aligned bounding box.
    ///   - chubbiness: Corner rounding amount.
    public func quadFill(_ aabb: AABB, chubbiness: Float = 0) {
        // In real implementation, this would queue a draw command
    }

    /// Draws a filled quad from four corners.
    public func quadFill(_ p0: Vector2, _ p1: Vector2, _ p2: Vector2, _ p3: Vector2,
                         chubbiness: Float = 0) {
        // In real implementation, this would queue a draw command
    }

    /// Draws a box wireframe with rounded corners.
    /// - Parameters:
    ///   - aabb: The axis-aligned bounding box.
    ///   - thickness: The line thickness.
    ///   - radius: The corner radius.
    public func boxRounded(_ aabb: AABB, thickness: Float, radius: Float) {
        // In real implementation, this would queue a draw command
    }

    /// Draws a filled box with rounded corners.
    /// - Parameters:
    ///   - aabb: The axis-aligned bounding box.
    ///   - radius: The corner radius.
    public func boxRoundedFill(_ aabb: AABB, radius: Float) {
        // In real implementation, this would queue a draw command
    }

    // MARK: - Circle Drawing

    /// Draws a circle wireframe.
    /// - Parameters:
    ///   - circle: The circle to draw.
    ///   - thickness: The line thickness.
    public func circle(_ circle: Circle, thickness: Float) {
        self.circle(center: circle.position, radius: circle.radius, thickness: thickness)
    }

    /// Draws a circle wireframe.
    /// - Parameters:
    ///   - center: The center position.
    ///   - radius: The radius.
    ///   - thickness: The line thickness.
    public func circle(center: Vector2, radius: Float, thickness: Float) {
        // In real implementation, this would queue a draw command
    }

    /// Draws a filled circle.
    /// - Parameter circle: The circle to draw.
    public func circleFill(_ circle: Circle) {
        circleFill(center: circle.position, radius: circle.radius)
    }

    /// Draws a filled circle.
    /// - Parameters:
    ///   - center: The center position.
    ///   - radius: The radius.
    public func circleFill(center: Vector2, radius: Float) {
        // In real implementation, this would queue a draw command
    }

    // MARK: - Capsule Drawing

    /// Draws a capsule wireframe.
    /// - Parameters:
    ///   - capsule: The capsule to draw.
    ///   - thickness: The line thickness.
    public func capsule(_ capsule: Capsule, thickness: Float) {
        self.capsule(from: capsule.a, to: capsule.b, radius: capsule.radius, thickness: thickness)
    }

    /// Draws a capsule wireframe.
    /// - Parameters:
    ///   - from: The center of one end cap.
    ///   - to: The center of the other end cap.
    ///   - radius: The capsule radius.
    ///   - thickness: The line thickness.
    public func capsule(from: Vector2, to: Vector2, radius: Float, thickness: Float) {
        // In real implementation, this would queue a draw command
    }

    /// Draws a filled capsule.
    /// - Parameter capsule: The capsule to draw.
    public func capsuleFill(_ capsule: Capsule) {
        capsuleFill(from: capsule.a, to: capsule.b, radius: capsule.radius)
    }

    /// Draws a filled capsule.
    /// - Parameters:
    ///   - from: The center of one end cap.
    ///   - to: The center of the other end cap.
    ///   - radius: The capsule radius.
    public func capsuleFill(from: Vector2, to: Vector2, radius: Float) {
        // In real implementation, this would queue a draw command
    }

    // MARK: - Triangle Drawing

    /// Draws a triangle wireframe.
    /// - Parameters:
    ///   - p0: First corner.
    ///   - p1: Second corner.
    ///   - p2: Third corner.
    ///   - thickness: The line thickness.
    ///   - chubbiness: Corner rounding amount.
    public func triangle(_ p0: Vector2, _ p1: Vector2, _ p2: Vector2,
                         thickness: Float, chubbiness: Float = 0) {
        // In real implementation, this would queue a draw command
    }

    /// Draws a filled triangle.
    /// - Parameters:
    ///   - p0: First corner.
    ///   - p1: Second corner.
    ///   - p2: Third corner.
    ///   - chubbiness: Corner rounding amount.
    public func triangleFill(_ p0: Vector2, _ p1: Vector2, _ p2: Vector2,
                             chubbiness: Float = 0) {
        // In real implementation, this would queue a draw command
    }

    // MARK: - Line Drawing

    /// Draws a line.
    /// - Parameters:
    ///   - from: The start point.
    ///   - to: The end point.
    ///   - thickness: The line thickness.
    public func line(from: Vector2, to: Vector2, thickness: Float) {
        // In real implementation, this would queue a draw command
    }

    /// Draws a polyline (connected line segments).
    /// - Parameters:
    ///   - points: The points to connect.
    ///   - thickness: The line thickness.
    ///   - loop: Whether to connect the last point to the first.
    public func polyline(_ points: [Vector2], thickness: Float, loop: Bool = false) {
        // In real implementation, this would queue a draw command
    }

    /// Draws a quadratic bezier curve.
    /// - Parameters:
    ///   - start: The start point.
    ///   - control: The control point.
    ///   - end: The end point.
    ///   - segments: Number of line segments to use.
    ///   - thickness: The line thickness.
    public func bezier(start: Vector2, control: Vector2, end: Vector2,
                       segments: Int = 20, thickness: Float) {
        // In real implementation, this would queue a draw command
    }

    /// Draws a cubic bezier curve.
    /// - Parameters:
    ///   - start: The start point.
    ///   - control1: The first control point.
    ///   - control2: The second control point.
    ///   - end: The end point.
    ///   - segments: Number of line segments to use.
    ///   - thickness: The line thickness.
    public func bezier(start: Vector2, control1: Vector2, control2: Vector2, end: Vector2,
                       segments: Int = 20, thickness: Float) {
        // In real implementation, this would queue a draw command
    }

    /// Draws an arrow.
    /// - Parameters:
    ///   - from: The start point.
    ///   - to: The end point (arrow head).
    ///   - thickness: The line thickness.
    ///   - arrowWidth: The width of the arrow head.
    public func arrow(from: Vector2, to: Vector2, thickness: Float, arrowWidth: Float) {
        // In real implementation, this would queue a draw command
    }

    // MARK: - Polygon Drawing

    /// Draws a filled polygon.
    ///
    /// - Note: Limited to 8 vertices maximum for SDF rendering.
    /// - Parameters:
    ///   - points: The polygon vertices (counter-clockwise).
    ///   - chubbiness: Corner rounding amount.
    public func polygonFill(_ points: [Vector2], chubbiness: Float = 0) {
        precondition(points.count <= 8, "Polygon fill is limited to 8 vertices. Use polygonFillSimple for more.")
        // In real implementation, this would queue a draw command
    }

    /// Draws a filled simple polygon.
    ///
    /// Unlike ``polygonFill(_:chubbiness:)``, this supports more than 8 vertices
    /// but does not support chubbiness or antialiasing.
    ///
    /// - Parameter points: The polygon vertices (counter-clockwise, no self-intersections).
    public func polygonFillSimple(_ points: [Vector2]) {
        // In real implementation, this would triangulate and draw
    }
}

// MARK: - Convenience Drawing Functions

/// Global function to get the shared draw context.
public var Draw: DrawContext { DrawContext.shared }

// MARK: - Free Functions for Convenient API

/// Draws a quad wireframe.
@inlinable
public func drawQuad(_ aabb: AABB, thickness: Float, chubbiness: Float = 0) {
    Draw.quad(aabb, thickness: thickness, chubbiness: chubbiness)
}

/// Draws a filled quad.
@inlinable
public func drawQuadFill(_ aabb: AABB, chubbiness: Float = 0) {
    Draw.quadFill(aabb, chubbiness: chubbiness)
}

/// Draws a circle wireframe.
@inlinable
public func drawCircle(center: Vector2, radius: Float, thickness: Float) {
    Draw.circle(center: center, radius: radius, thickness: thickness)
}

/// Draws a filled circle.
@inlinable
public func drawCircleFill(center: Vector2, radius: Float) {
    Draw.circleFill(center: center, radius: radius)
}

/// Draws a line.
@inlinable
public func drawLine(from: Vector2, to: Vector2, thickness: Float) {
    Draw.line(from: from, to: to, thickness: thickness)
}

/// Draws a triangle wireframe.
@inlinable
public func drawTriangle(_ p0: Vector2, _ p1: Vector2, _ p2: Vector2,
                         thickness: Float, chubbiness: Float = 0) {
    Draw.triangle(p0, p1, p2, thickness: thickness, chubbiness: chubbiness)
}

/// Draws a filled triangle.
@inlinable
public func drawTriangleFill(_ p0: Vector2, _ p1: Vector2, _ p2: Vector2,
                             chubbiness: Float = 0) {
    Draw.triangleFill(p0, p1, p2, chubbiness: chubbiness)
}

/// Draws a capsule wireframe.
@inlinable
public func drawCapsule(from: Vector2, to: Vector2, radius: Float, thickness: Float) {
    Draw.capsule(from: from, to: to, radius: radius, thickness: thickness)
}

/// Draws a filled capsule.
@inlinable
public func drawCapsuleFill(from: Vector2, to: Vector2, radius: Float) {
    Draw.capsuleFill(from: from, to: to, radius: radius)
}
