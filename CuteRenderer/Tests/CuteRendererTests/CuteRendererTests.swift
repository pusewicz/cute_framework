import XCTest
@testable import CuteRenderer

final class CuteRendererTests: XCTestCase {
    // MARK: - Vector2 Tests

    func testVector2Creation() {
        let v = Vector2(x: 3, y: 4)
        XCTAssertEqual(v.x, 3)
        XCTAssertEqual(v.y, 4)
    }

    func testVector2Length() {
        let v = Vector2(x: 3, y: 4)
        XCTAssertEqual(v.length, 5)
    }

    func testVector2Normalized() {
        let v = Vector2(x: 3, y: 4)
        let n = v.normalized()
        XCTAssertEqual(n.length, 1, accuracy: 0.0001)
    }

    func testVector2Operations() {
        let a = Vector2(x: 1, y: 2)
        let b = Vector2(x: 3, y: 4)

        XCTAssertEqual(a + b, Vector2(x: 4, y: 6))
        XCTAssertEqual(b - a, Vector2(x: 2, y: 2))
        XCTAssertEqual(a * 2, Vector2(x: 2, y: 4))
        XCTAssertEqual(a.dot(b), 11) // 1*3 + 2*4
    }

    // MARK: - Color Tests

    func testColorCreation() {
        let c = Color(r: 1, g: 0.5, b: 0, a: 1)
        XCTAssertEqual(c.r, 1)
        XCTAssertEqual(c.g, 0.5)
        XCTAssertEqual(c.b, 0)
        XCTAssertEqual(c.a, 1)
    }

    func testColorFromHex() {
        let c = Color(hex: 0xFF5500)
        XCTAssertEqual(c.r, 1.0, accuracy: 0.01)
        XCTAssertEqual(c.g, 0.333, accuracy: 0.01)
        XCTAssertEqual(c.b, 0, accuracy: 0.01)
    }

    func testColorLerp() {
        let black = Color.black
        let white = Color.white
        let gray = black.lerp(to: white, t: 0.5)
        XCTAssertEqual(gray.r, 0.5, accuracy: 0.01)
        XCTAssertEqual(gray.g, 0.5, accuracy: 0.01)
        XCTAssertEqual(gray.b, 0.5, accuracy: 0.01)
    }

    // MARK: - Transform Tests

    func testTransformIdentity() {
        let t = Transform.identity
        let p = Vector2(x: 5, y: 10)
        let transformed = t.transformPoint(p)
        XCTAssertEqual(transformed.x, p.x, accuracy: 0.0001)
        XCTAssertEqual(transformed.y, p.y, accuracy: 0.0001)
    }

    func testTransformRotation() {
        let t = Transform(radians: .pi / 2, position: .zero) // 90 degrees
        let p = Vector2(x: 1, y: 0)
        let transformed = t.transformPoint(p)
        XCTAssertEqual(transformed.x, 0, accuracy: 0.0001)
        XCTAssertEqual(transformed.y, 1, accuracy: 0.0001)
    }

    // MARK: - AABB Tests

    func testAABBContains() {
        let aabb = AABB(min: Vector2(x: 0, y: 0), max: Vector2(x: 10, y: 10))
        XCTAssertTrue(aabb.contains(Vector2(x: 5, y: 5)))
        XCTAssertFalse(aabb.contains(Vector2(x: 15, y: 5)))
    }

    func testAABBOverlaps() {
        let a = AABB(min: Vector2(x: 0, y: 0), max: Vector2(x: 10, y: 10))
        let b = AABB(min: Vector2(x: 5, y: 5), max: Vector2(x: 15, y: 15))
        let c = AABB(min: Vector2(x: 20, y: 20), max: Vector2(x: 30, y: 30))
        XCTAssertTrue(a.overlaps(b))
        XCTAssertFalse(a.overlaps(c))
    }

    // MARK: - Sprite Tests

    func testSpriteDefaults() {
        let sprite = Sprite()
        XCTAssertEqual(sprite.scale, .one)
        XCTAssertEqual(sprite.opacity, 1)
        XCTAssertTrue(sprite.loop)
        XCTAssertFalse(sprite.paused)
    }

    func testSpriteAnimation() {
        var sprite = Sprite()
        let animation = Animation(
            name: "walk",
            playDirection: .forwards,
            frames: [
                AnimationFrame(imageID: 1, delay: 0.1),
                AnimationFrame(imageID: 2, delay: 0.1),
                AnimationFrame(imageID: 3, delay: 0.1),
            ]
        )
        sprite.animations["walk"] = animation
        sprite.play("walk")

        XCTAssertEqual(sprite.frameIndex, 0)

        sprite.update(deltaTime: 0.15) // Should advance to frame 1
        XCTAssertEqual(sprite.frameIndex, 1)

        sprite.update(deltaTime: 0.15) // Should advance to frame 2
        XCTAssertEqual(sprite.frameIndex, 2)
    }

    // MARK: - Matrix3x2 Tests

    func testMatrix3x2Identity() {
        let m = Matrix3x2.identity
        let p = Vector2(x: 5, y: 10)
        let result = m * p
        XCTAssertEqual(result.x, p.x, accuracy: 0.0001)
        XCTAssertEqual(result.y, p.y, accuracy: 0.0001)
    }

    func testMatrix3x2Translation() {
        let m = Matrix3x2.translation(x: 10, y: 20)
        let p = Vector2(x: 5, y: 5)
        let result = m * p
        XCTAssertEqual(result.x, 15, accuracy: 0.0001)
        XCTAssertEqual(result.y, 25, accuracy: 0.0001)
    }

    func testMatrix3x2Scale() {
        let m = Matrix3x2.scale(2)
        let p = Vector2(x: 5, y: 10)
        let result = m * p
        XCTAssertEqual(result.x, 10, accuracy: 0.0001)
        XCTAssertEqual(result.y, 20, accuracy: 0.0001)
    }
}
