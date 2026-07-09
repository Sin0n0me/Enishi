#pragma once

namespace enishi::renderer {
    namespace directx {
        enum class DirectXError {
            DeviceError,
            FactoryError,
            SurfaceError,
            SwapchainError,
            TargetError,
            VisualError,
            ShaderError,
            ShaderReflectionError,
            BufferError,
            ViewError,
            RasterizerError,
            InputLayoutError,
        };
    } // namespace directx

    namespace opengl {} // namespace opengl

    namespace vulkan {} // namespace vulkan
} // namespace enishi::renderer