import CSDL3

// MARK: - SDL_GPUDevice Extensions

extension OpaquePointer {
    /// Creates a texture on the GPU device.
    /// - Parameters:
    ///   - width: Width in pixels.
    ///   - height: Height in pixels.
    ///   - format: Pixel format (default: RGBA8).
    ///   - usage: Usage flags (default: sampler).
    /// - Returns: The created texture, or nil on failure.
    @inlinable
    public func createTexture(
        width: Int,
        height: Int,
        format: SDL_GPUTextureFormat = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM,
        usage: SDL_GPUTextureUsageFlags = SDL_GPU_TEXTUREUSAGE_SAMPLER
    ) -> OpaquePointer? {
        var info = SDL_GPUTextureCreateInfo()
        info.type = SDL_GPU_TEXTURETYPE_2D
        info.format = format
        info.usage = usage
        info.width = UInt32(width)
        info.height = UInt32(height)
        info.layer_count_or_depth = 1
        info.num_levels = 1
        info.sample_count = SDL_GPU_SAMPLECOUNT_1
        return SDL_CreateGPUTexture(self, &info)
    }

    /// Creates a sampler on the GPU device.
    @inlinable
    public func createSampler(
        minFilter: SDL_GPUFilter = SDL_GPU_FILTER_NEAREST,
        magFilter: SDL_GPUFilter = SDL_GPU_FILTER_NEAREST,
        addressModeU: SDL_GPUSamplerAddressMode = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
        addressModeV: SDL_GPUSamplerAddressMode = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE
    ) -> OpaquePointer? {
        var info = SDL_GPUSamplerCreateInfo()
        info.min_filter = minFilter
        info.mag_filter = magFilter
        info.address_mode_u = addressModeU
        info.address_mode_v = addressModeV
        info.mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_NEAREST
        return SDL_CreateGPUSampler(self, &info)
    }

    /// Creates a graphics pipeline.
    @inlinable
    public func createGraphicsPipeline(
        vertexShader: OpaquePointer,
        fragmentShader: OpaquePointer,
        vertexInputState: SDL_GPUVertexInputState,
        primitiveType: SDL_GPUPrimitiveType = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST,
        rasterizerState: SDL_GPURasterizerState? = nil,
        depthStencilState: SDL_GPUDepthStencilState? = nil,
        targetInfo: SDL_GPUGraphicsPipelineTargetInfo
    ) -> OpaquePointer? {
        var info = SDL_GPUGraphicsPipelineCreateInfo()
        info.vertex_shader = vertexShader
        info.fragment_shader = fragmentShader
        info.vertex_input_state = vertexInputState
        info.primitive_type = primitiveType
        info.target_info = targetInfo

        if var rasterizer = rasterizerState {
            info.rasterizer_state = rasterizer
        }
        if var depthStencil = depthStencilState {
            info.depth_stencil_state = depthStencil
        }

        return SDL_CreateGPUGraphicsPipeline(self, &info)
    }

    /// Creates a shader from SPIRV bytecode.
    @inlinable
    public func createShader(
        bytecode: UnsafePointer<UInt8>,
        bytecodeSize: Int,
        stage: SDL_GPUShaderStage,
        entryPoint: String = "main",
        samplerCount: Int = 0,
        uniformBufferCount: Int = 0
    ) -> OpaquePointer? {
        var info = SDL_GPUShaderCreateInfo()
        info.code = bytecode
        info.code_size = bytecodeSize
        info.stage = stage
        info.format = SDL_GPU_SHADERFORMAT_SPIRV
        info.num_samplers = UInt32(samplerCount)
        info.num_uniform_buffers = UInt32(uniformBufferCount)

        return entryPoint.withCString { entryPointPtr in
            info.entrypoint = entryPointPtr
            return SDL_CreateGPUShader(self, &info)
        }
    }

    /// Creates a GPU buffer.
    @inlinable
    public func createBuffer(
        size: Int,
        usage: SDL_GPUBufferUsageFlags
    ) -> OpaquePointer? {
        var info = SDL_GPUBufferCreateInfo()
        info.usage = usage
        info.size = UInt32(size)
        return SDL_CreateGPUBuffer(self, &info)
    }

    /// Creates a transfer buffer for uploading data.
    @inlinable
    public func createTransferBuffer(size: Int, usage: SDL_GPUTransferBufferUsage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD) -> OpaquePointer? {
        var info = SDL_GPUTransferBufferCreateInfo()
        info.usage = usage
        info.size = UInt32(size)
        return SDL_CreateGPUTransferBuffer(self, &info)
    }

    /// Acquires a command buffer.
    @inlinable
    public func acquireCommandBuffer() -> OpaquePointer? {
        return SDL_AcquireGPUCommandBuffer(self)
    }
}

// MARK: - SDL_GPUCommandBuffer Extensions

extension OpaquePointer {
    /// Begins a render pass.
    @inlinable
    public func beginRenderPass(
        colorTargets: inout [SDL_GPUColorTargetInfo],
        depthStencilTarget: UnsafePointer<SDL_GPUDepthStencilTargetInfo>? = nil
    ) -> OpaquePointer? {
        return colorTargets.withUnsafeMutableBufferPointer { buffer in
            SDL_BeginGPURenderPass(self, buffer.baseAddress, UInt32(buffer.count), depthStencilTarget)
        }
    }

    /// Submits the command buffer.
    @inlinable
    public func submit() -> Bool {
        return SDL_SubmitGPUCommandBuffer(self)
    }
}

// MARK: - Texture Format Helpers

extension SDL_GPUTextureFormat {
    /// RGBA 8-bit unsigned normalized.
    public static let rgba8Unorm = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM

    /// BGRA 8-bit unsigned normalized.
    public static let bgra8Unorm = SDL_GPU_TEXTUREFORMAT_B8G8R8A8_UNORM

    /// RGBA 8-bit sRGB.
    public static let rgba8Srgb = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM_SRGB

    /// Depth 24-bit with 8-bit stencil.
    public static let depth24Stencil8 = SDL_GPU_TEXTUREFORMAT_D24_UNORM_S8_UINT

    /// Depth 32-bit float.
    public static let depth32Float = SDL_GPU_TEXTUREFORMAT_D32_FLOAT
}

// MARK: - Filter Helpers

extension SDL_GPUFilter {
    /// Nearest neighbor filtering (good for pixel art).
    public static let nearest = SDL_GPU_FILTER_NEAREST

    /// Linear filtering (smooth scaling).
    public static let linear = SDL_GPU_FILTER_LINEAR
}

// MARK: - Blend Factor Helpers

extension SDL_GPUBlendFactor {
    public static let zero = SDL_GPU_BLENDFACTOR_ZERO
    public static let one = SDL_GPU_BLENDFACTOR_ONE
    public static let srcColor = SDL_GPU_BLENDFACTOR_SRC_COLOR
    public static let oneMinusSrcColor = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_COLOR
    public static let dstColor = SDL_GPU_BLENDFACTOR_DST_COLOR
    public static let oneMinusDstColor = SDL_GPU_BLENDFACTOR_ONE_MINUS_DST_COLOR
    public static let srcAlpha = SDL_GPU_BLENDFACTOR_SRC_ALPHA
    public static let oneMinusSrcAlpha = SDL_GPU_BLENDFACTOR_ONE_MINUS_SRC_ALPHA
    public static let dstAlpha = SDL_GPU_BLENDFACTOR_DST_ALPHA
    public static let oneMinusDstAlpha = SDL_GPU_BLENDFACTOR_ONE_MINUS_DST_ALPHA
}

// MARK: - Color Conversion

extension SDL_FColor {
    /// Creates an SDL_FColor from a Color.
    @inlinable
    public init(_ color: Color) {
        self.init(r: color.r, g: color.g, b: color.b, a: color.a)
    }
}

extension Color {
    /// Creates a Color from an SDL_FColor.
    @inlinable
    public init(_ sdlColor: SDL_FColor) {
        self.init(r: sdlColor.r, g: sdlColor.g, b: sdlColor.b, a: sdlColor.a)
    }

    /// Converts to SDL_FColor.
    @inlinable
    public var sdlColor: SDL_FColor {
        SDL_FColor(r: r, g: g, b: b, a: a)
    }
}

// MARK: - Vector Conversion

extension SDL_FPoint {
    /// Creates an SDL_FPoint from a Vector2.
    @inlinable
    public init(_ v: Vector2) {
        self.init(x: v.x, y: v.y)
    }
}

extension Vector2 {
    /// Creates a Vector2 from an SDL_FPoint.
    @inlinable
    public init(_ p: SDL_FPoint) {
        self.init(x: p.x, y: p.y)
    }

    /// Converts to SDL_FPoint.
    @inlinable
    public var sdlPoint: SDL_FPoint {
        SDL_FPoint(x: x, y: y)
    }
}

// MARK: - Rect Conversion

extension SDL_Rect {
    /// Creates an SDL_Rect from a Rect.
    @inlinable
    public init(_ r: Rect) {
        self.init(x: Int32(r.x), y: Int32(r.y), w: Int32(r.width), h: Int32(r.height))
    }
}

extension SDL_FRect {
    /// Creates an SDL_FRect from an AABB.
    @inlinable
    public init(_ aabb: AABB) {
        self.init(x: aabb.min.x, y: aabb.min.y, w: aabb.width, h: aabb.height)
    }
}

extension Rect {
    /// Creates a Rect from an SDL_Rect.
    @inlinable
    public init(_ r: SDL_Rect) {
        self.init(x: Int(r.x), y: Int(r.y), width: Int(r.w), height: Int(r.h))
    }

    /// Converts to SDL_Rect.
    @inlinable
    public var sdlRect: SDL_Rect {
        SDL_Rect(x: Int32(x), y: Int32(y), w: Int32(width), h: Int32(height))
    }
}

extension AABB {
    /// Creates an AABB from an SDL_FRect.
    @inlinable
    public init(_ r: SDL_FRect) {
        self.init(
            min: Vector2(x: r.x, y: r.y),
            max: Vector2(x: r.x + r.w, y: r.y + r.h)
        )
    }

    /// Converts to SDL_FRect.
    @inlinable
    public var sdlRect: SDL_FRect {
        SDL_FRect(x: min.x, y: min.y, w: width, h: height)
    }
}
