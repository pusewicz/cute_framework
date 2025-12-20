/// The direction an animation plays.
public enum PlayDirection: Int, Hashable, Codable, Sendable {
    /// Play frames forward (0, 1, 2, ...).
    case forwards = 0

    /// Play frames backward (..., 2, 1, 0).
    case backwards = 1

    /// Play forward then backward, repeating.
    case pingpong = 2
}

/// A single frame of animation.
public struct AnimationFrame: Hashable, Codable, Sendable {
    /// Unique identifier for this frame's image.
    public var imageID: UInt64

    /// Duration to display this frame in seconds.
    public var delay: Float

    /// Creates an animation frame.
    public init(imageID: UInt64, delay: Float) {
        self.imageID = imageID
        self.delay = delay
    }
}

/// A named animation containing a sequence of frames.
public struct Animation: Hashable, Sendable {
    /// The animation name.
    public var name: String

    /// The play direction.
    public var playDirection: PlayDirection

    /// The frames in this animation.
    public var frames: [AnimationFrame]

    /// Internal offset for frame indices.
    internal var frameOffset: Int

    /// Creates an animation.
    public init(name: String, playDirection: PlayDirection = .forwards, frames: [AnimationFrame] = []) {
        self.name = name
        self.playDirection = playDirection
        self.frames = frames
        self.frameOffset = 0
    }

    /// The total duration of the animation in seconds.
    public var duration: Float {
        return frames.reduce(0) { $0 + $1.delay }
    }

    /// The number of frames.
    public var frameCount: Int {
        return frames.count
    }

    /// Adds a frame to the animation.
    public mutating func addFrame(_ frame: AnimationFrame) {
        frames.append(frame)
    }
}

/// A slice defined within a sprite (from Aseprite).
public struct SpriteSlice: Hashable, Codable, Sendable {
    /// The frame index this slice starts at.
    public var frameIndex: Int

    /// The slice name.
    public var name: String

    /// The bounding box of the slice.
    public var bounds: AABB

    /// Creates a sprite slice.
    public init(frameIndex: Int, name: String, bounds: AABB) {
        self.frameIndex = frameIndex
        self.name = name
        self.bounds = bounds
    }
}

/// A drawable sprite with animation support.
///
/// Sprites can be created from PNG files (single-frame) or Aseprite files (animated).
/// They support animations, 9-slice rendering, and various visual transforms.
///
/// ## Topics
///
/// ### Creating Sprites
/// - ``init(pngPath:)``
/// - ``init(asepritePath:)``
/// - ``init(pixels:width:height:)``
///
/// ### Animation
/// - ``play(_:)``
/// - ``update()``
/// - ``pause()``
/// - ``unpause()``
///
/// ### Properties
/// - ``width``
/// - ``height``
/// - ``scale``
/// - ``offset``
/// - ``opacity``
///
/// ### Drawing
/// - ``draw()``
/// - ``draw9Slice()``
/// - ``draw9SliceTiled()``
public struct Sprite: Hashable, Sendable {
    /// The sprite name.
    public var name: String?

    /// Width in pixels.
    public var width: Int

    /// Height in pixels.
    public var height: Int

    /// Scale factor for drawing.
    public var scale: Vector2

    /// Local offset/origin for drawing.
    public var offset: Vector2

    /// Per-frame pivot points.
    public var pivots: [Vector2]

    /// Center patches for 9-slice rendering.
    public var centerPatches: [AABB]

    /// Slices defined in the sprite.
    public var slices: [SpriteSlice]

    /// Opacity (0-1).
    public var opacity: Float

    /// Current frame index.
    public var frameIndex: Int

    /// Number of completed animation loops.
    public var loopCount: Int

    /// Animation playback speed multiplier.
    public var playSpeedMultiplier: Float

    /// Whether animation is paused.
    public var paused: Bool

    /// Whether the animation loops.
    public var loop: Bool

    /// Elapsed time in current frame.
    public var elapsedTime: Float

    /// Internal ID for easy sprites.
    internal var easySpriteID: UInt64

    /// Current play direction.
    public var playDirection: PlayDirection

    /// Current animation.
    public var animation: Animation?

    /// All animations keyed by name.
    public var animations: [String: Animation]

    /// Transform for rendering.
    public var transform: Transform

    /// Creates an empty sprite with default values.
    public init() {
        self.name = nil
        self.width = 0
        self.height = 0
        self.scale = .one
        self.offset = .zero
        self.pivots = []
        self.centerPatches = []
        self.slices = []
        self.opacity = 1
        self.frameIndex = 0
        self.loopCount = 0
        self.playSpeedMultiplier = 1
        self.paused = false
        self.loop = true
        self.elapsedTime = 0
        self.easySpriteID = 0
        self.playDirection = .forwards
        self.animation = nil
        self.animations = [:]
        self.transform = .identity
    }

    // MARK: - Factory Methods

    /// Creates a sprite from a PNG file.
    /// - Parameter path: Virtual path to the PNG file.
    /// - Returns: A single-frame sprite.
    public static func fromPNG(path: String) -> Sprite? {
        // In real implementation, this would load the PNG
        var sprite = Sprite()
        sprite.name = path
        return sprite
    }

    /// Creates a sprite from pixel data.
    /// - Parameters:
    ///   - pixels: Array of pixel colors.
    ///   - width: Width in pixels.
    ///   - height: Height in pixels.
    /// - Returns: A single-frame sprite.
    public static func fromPixels(_ pixels: [Pixel], width: Int, height: Int) -> Sprite {
        var sprite = Sprite()
        sprite.width = width
        sprite.height = height
        return sprite
    }

    /// Creates a sprite from an Aseprite file.
    /// - Parameter path: Virtual path to the .ase/.aseprite file.
    /// - Returns: An animated sprite with all animations loaded.
    public static func fromAseprite(path: String) -> Sprite? {
        // In real implementation, this would parse the Aseprite file
        var sprite = Sprite()
        sprite.name = path
        return sprite
    }

    // MARK: - Animation Control

    /// Plays an animation by name.
    /// - Parameter animationName: The name of the animation to play.
    public mutating func play(_ animationName: String) {
        guard let anim = animations[animationName] else { return }
        animation = anim
        reset()
    }

    /// Resets the current animation to the beginning.
    public mutating func reset() {
        paused = false
        frameIndex = 0
        loopCount = 0
        elapsedTime = 0
        if let anim = animation {
            playDirection = anim.playDirection
        }
    }

    /// Updates the animation state.
    ///
    /// Call this once per frame with your delta time.
    public mutating func update(deltaTime: Float) {
        guard !paused, let anim = animation, !anim.frames.isEmpty else { return }

        elapsedTime += deltaTime * playSpeedMultiplier
        let frameCount = anim.frames.count

        switch playDirection {
        case .forwards:
            if elapsedTime >= anim.frames[frameIndex].delay {
                frameIndex += 1
                elapsedTime = 0
                if frameIndex >= frameCount {
                    if loop {
                        loopCount += 1
                        frameIndex = 0
                    } else {
                        frameIndex = frameCount - 1
                    }
                }
            }

        case .backwards:
            if elapsedTime >= anim.frames[frameIndex].delay {
                frameIndex -= 1
                elapsedTime = 0
                if frameIndex < 0 {
                    if loop {
                        loopCount += 1
                        frameIndex = frameCount - 1
                    } else {
                        frameIndex = 0
                    }
                }
            }

        case .pingpong:
            if elapsedTime >= anim.frames[frameIndex].delay {
                elapsedTime = 0
                if loopCount % 2 == 1 {
                    frameIndex -= 1
                    if frameIndex < 0 {
                        if loop {
                            loopCount += 1
                            frameIndex = 0
                        } else {
                            frameIndex = 0
                        }
                    }
                } else {
                    frameIndex += 1
                    if frameIndex >= frameCount {
                        loopCount += 1
                        frameIndex = frameCount - 1
                    }
                }
            }
        }
    }

    /// Pauses the animation.
    public mutating func pause() {
        paused = true
    }

    /// Unpauses the animation.
    public mutating func unpause() {
        paused = false
    }

    /// Toggles the pause state.
    public mutating func togglePause() {
        paused.toggle()
    }

    /// Flips the sprite horizontally.
    public mutating func flipX() {
        scale.x *= -1
    }

    /// Flips the sprite vertically.
    public mutating func flipY() {
        scale.y *= -1
    }

    // MARK: - Queries

    /// Returns true if the specified animation is currently playing.
    public func isPlaying(_ animationName: String) -> Bool {
        return animation?.name == animationName
    }

    /// Gets a slice by name for the current frame.
    public func slice(named name: String) -> AABB? {
        let currentGlobalFrame = frameIndex + (animation?.frameOffset ?? 0)
        return slices.first { $0.name == name && $0.frameIndex <= currentGlobalFrame }?.bounds
    }

    /// The number of frames in the current animation.
    public var frameCount: Int {
        return animation?.frames.count ?? 0
    }

    /// The current frame delay.
    public var currentFrameDelay: Float {
        guard let anim = animation, frameIndex < anim.frames.count else { return 0 }
        return anim.frames[frameIndex].delay
    }

    /// The total duration of the current animation.
    public var animationDuration: Float {
        return animation?.duration ?? 0
    }

    /// The interpolation progress through the animation (0-1).
    public var animationProgress: Float {
        guard let anim = animation, !anim.frames.isEmpty else { return 0 }
        let duration = anim.duration
        guard duration > 0 else { return 0 }
        var elapsed: Float = elapsedTime
        for i in 0..<frameIndex {
            elapsed += anim.frames[i].delay
        }
        return min(1, max(0, elapsed / duration))
    }

    /// Returns true if the animation will complete on the next update.
    public func willFinish(deltaTime: Float) -> Bool {
        guard let anim = animation, frameIndex == anim.frames.count - 1 else { return false }
        return elapsedTime + deltaTime * playSpeedMultiplier >= anim.frames[frameIndex].delay
    }

    /// Returns true if the animation just looped.
    public var onLoop: Bool {
        return frameIndex == 0 && elapsedTime == 0
    }

    // MARK: - Drawing

    /// Draws the sprite.
    public func draw() {
        Draw.sprite(self)
    }

    /// Draws the sprite using 9-slice scaling.
    public func draw9Slice() {
        Draw.sprite9Slice(self)
    }

    /// Draws the sprite using tiled 9-slice.
    public func draw9SliceTiled() {
        Draw.sprite9SliceTiled(self)
    }
}

// MARK: - Renderer Sprite Extension

extension Renderer {
    /// Draws a sprite.
    public func sprite(_ sprite: Sprite) {
        // TODO: Queue sprite draw command for batching
    }

    /// Draws a sprite using 9-slice scaling.
    public func sprite9Slice(_ sprite: Sprite) {
        // TODO: Queue 9-slice draw command for batching
    }

    /// Draws a sprite using tiled 9-slice.
    public func sprite9SliceTiled(_ sprite: Sprite) {
        // TODO: Queue tiled 9-slice draw command for batching
    }

    /// Prefetches a sprite without drawing.
    public func prefetch(_ sprite: Sprite) {
        // TODO: Ensure the sprite texture is loaded
    }
}

// MARK: - Free Functions

/// Draws a sprite.
@inlinable
public func drawSprite(_ sprite: Sprite) {
    Draw.sprite(sprite)
}

/// Draws a sprite using 9-slice scaling.
@inlinable
public func drawSprite9Slice(_ sprite: Sprite) {
    Draw.sprite9Slice(sprite)
}
