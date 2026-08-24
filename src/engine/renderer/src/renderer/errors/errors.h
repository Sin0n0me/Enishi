#pragma once

namespace enishi::renderer {
    enum class RendererError {
        DeviceError,
        FactoryError,
        SurfaceError,
        SwapchainError,
        ViewError,
        VisualError,
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