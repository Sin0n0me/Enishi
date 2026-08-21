#pragma once

namespace enishi::renderer {
    enum class RendererError {
        DeviceError,
        FactoryError,
        SurfaceError,
        SwapchainError,
        ViewError,
        VisualError,
        ShaderError,
        ShaderReflectionError,
        BufferError,
        RasterizerError,
        InputLayoutError,
    };
} // namespace enishi::renderer