/// The pixel format for GPU textures.
public enum PixelFormat: Int, Hashable, Codable, Sendable {
    /// Invalid pixel format.
    case invalid = -1

    /// 8-bit alpha channel.
    case a8Unorm = 0

    /// 8-bit red channel.
    case r8Unorm = 1

    /// 8-bit red/green channels.
    case r8g8Unorm = 2

    /// 8-bit RGBA (most common format).
    case r8g8b8a8Unorm = 3

    /// 16-bit red channel.
    case r16Unorm = 4

    /// 16-bit red/green channels.
    case r16g16Unorm = 5

    /// 16-bit RGBA.
    case r16g16b16a16Unorm = 6

    /// 32-bit RGBA float.
    case r32g32b32a32Float = 31

    /// sRGB RGBA format.
    case r8g8b8a8UnormSRGB = 45

    /// 16-bit depth.
    case d16Unorm = 51

    /// 24-bit depth.
    case d24Unorm = 52

    /// 32-bit depth float.
    case d32Float = 53

    /// 24-bit depth with 8-bit stencil.
    case d24UnormS8Uint = 54

    /// 32-bit depth float with 8-bit stencil.
    case d32FloatS8Uint = 55
}

/// Texture filtering mode.
public enum TextureFilter: Int, Hashable, Codable, Sendable {
    /// Nearest-neighbor filtering (good for pixel art).
    case nearest = 0

    /// Linear (bilinear) filtering (smooth scaling).
    case linear = 1
}

/// Mipmap filtering mode.
public enum MipFilter: Int, Hashable, Codable, Sendable {
    /// Sample nearest mip level.
    case nearest = 0

    /// Linear blend between mip levels.
    case linear = 1
}

/// Texture wrap mode for coordinates outside 0-1 range.
public enum WrapMode: Int, Hashable, Codable, Sendable {
    /// Repeat the texture.
    case `repeat` = 0

    /// Clamp to edge pixels.
    case clampToEdge = 1

    /// Repeat with mirroring.
    case mirroredRepeat = 2
}

/// Texture usage flags.
public struct TextureUsage: OptionSet, Hashable, Codable, Sendable {
    public let rawValue: UInt32

    public init(rawValue: UInt32) {
        self.rawValue = rawValue
    }

    /// The texture can be sampled in shaders.
    public static let sampler = TextureUsage(rawValue: 0x00000001)

    /// The texture can be used as a color render target.
    public static let colorTarget = TextureUsage(rawValue: 0x00000002)

    /// The texture can be used as a depth/stencil target.
    public static let depthStencilTarget = TextureUsage(rawValue: 0x00000004)

    /// The texture can be read in graphics pipelines.
    public static let graphicsStorageRead = TextureUsage(rawValue: 0x00000008)

    /// The texture can be read in compute pipelines.
    public static let computeStorageRead = TextureUsage(rawValue: 0x00000020)

    /// The texture can be written in compute pipelines.
    public static let computeStorageWrite = TextureUsage(rawValue: 0x00000040)

    /// Default usage: sampler and color target.
    public static let `default`: TextureUsage = [.sampler, .colorTarget]
}

/// Parameters for creating a texture.
public struct TextureParams: Hashable, Sendable {
    /// The pixel format.
    public var pixelFormat: PixelFormat

    /// Usage flags.
    public var usage: TextureUsage

    /// Filtering mode.
    public var filter: TextureFilter

    /// Wrap mode for U coordinate.
    public var wrapU: WrapMode

    /// Wrap mode for V coordinate.
    public var wrapV: WrapMode

    /// Mipmap filter mode.
    public var mipFilter: MipFilter

    /// Width in pixels.
    public var width: Int

    /// Height in pixels.
    public var height: Int

    /// Number of mip levels (0 = auto).
    public var mipCount: Int

    /// Whether to generate mipmaps.
    public var generateMipmaps: Bool

    /// Mipmap LOD bias.
    public var mipLodBias: Float

    /// Maximum anisotropy level.
    public var maxAnisotropy: Float

    /// Whether this texture is updated frequently.
    public var stream: Bool

    /// Creates texture parameters with sensible defaults.
    public init(
        width: Int,
        height: Int,
        pixelFormat: PixelFormat = .r8g8b8a8Unorm,
        usage: TextureUsage = .default,
        filter: TextureFilter = .nearest,
        wrapU: WrapMode = .clampToEdge,
        wrapV: WrapMode = .clampToEdge,
        mipFilter: MipFilter = .linear,
        mipCount: Int = 0,
        generateMipmaps: Bool = false,
        mipLodBias: Float = 0,
        maxAnisotropy: Float = 1,
        stream: Bool = false
    ) {
        self.width = width
        self.height = height
        self.pixelFormat = pixelFormat
        self.usage = usage
        self.filter = filter
        self.wrapU = wrapU
        self.wrapV = wrapV
        self.mipFilter = mipFilter
        self.mipCount = mipCount
        self.generateMipmaps = generateMipmaps
        self.mipLodBias = mipLodBias
        self.maxAnisotropy = maxAnisotropy
        self.stream = stream
    }

    /// Returns parameters optimized for pixel art.
    public static func pixelArt(width: Int, height: Int) -> TextureParams {
        return TextureParams(
            width: width,
            height: height,
            filter: .nearest
        )
    }

    /// Returns parameters optimized for smooth graphics.
    public static func smooth(width: Int, height: Int) -> TextureParams {
        return TextureParams(
            width: width,
            height: height,
            filter: .linear,
            generateMipmaps: true
        )
    }
}

/// A GPU texture handle.
///
/// Textures are buffers of data on the GPU, typically used for image data.
/// They can be sampled in shaders or used as render targets.
///
/// ## Topics
///
/// ### Creating Textures
/// - ``init(params:)``
/// - ``init(params:data:)``
/// - ``init(width:height:pixels:)``
///
/// ### Updating Textures
/// - ``update(data:)``
/// - ``generateMipmaps()``
///
/// ### Properties
/// - ``width``
/// - ``height``
/// - ``size``
public final class Texture: Hashable, Sendable {
    /// Internal handle (would be SDL_GPUTexture* in real implementation).
    internal let handle: UInt64

    /// The texture parameters.
    public let params: TextureParams

    /// The width in pixels.
    public var width: Int { params.width }

    /// The height in pixels.
    public var height: Int { params.height }

    /// The size as a Vector2.
    public var size: Vector2 {
        Vector2(x: Float(width), y: Float(height))
    }

    /// Creates a texture with the specified parameters.
    public init(params: TextureParams) {
        self.params = params
        // In real implementation, this would create SDL_GPUTexture
        self.handle = UInt64.random(in: 1...UInt64.max)
    }

    /// Creates a texture with data.
    public convenience init(params: TextureParams, data: Data) {
        self.init(params: params)
        update(data: data)
    }

    /// Creates a texture from pixel data.
    public convenience init(width: Int, height: Int, pixels: [Pixel]) {
        let params = TextureParams(width: width, height: height)
        self.init(params: params)

        let data = pixels.withUnsafeBytes { Data($0) }
        update(data: data)
    }

    /// Updates the texture contents.
    public func update(data: Data) {
        // In real implementation, this would call cf_texture_update
    }

    /// Updates a specific mip level.
    public func update(data: Data, mipLevel: Int) {
        // In real implementation, this would call cf_texture_update_mip
    }

    /// Generates mipmaps from the base level.
    public func generateMipmaps() {
        // In real implementation, this would call cf_generate_mipmaps
    }

    // MARK: - Hashable

    public static func == (lhs: Texture, rhs: Texture) -> Bool {
        return lhs.handle == rhs.handle
    }

    public func hash(into hasher: inout Hasher) {
        hasher.combine(handle)
    }
}
