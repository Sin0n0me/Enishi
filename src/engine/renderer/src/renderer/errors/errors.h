#pragma once

namespace enishi::renderer {
    enum class RendererError {
        DeviceError,
        FactoryError,
        SurfaceError,
        SwapchainError,
        ViewError,
        VisualError,
        ImageError,
        TextureError,
        SamplerError,
        ShaderError,
        ShaderReflectionError,
        BufferError,
        RasterizerError,
        InputLayoutError,
        ConvertError,
    };
} // namespace enishi::renderer