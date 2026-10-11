#pragma once
#include <cstdint>

namespace enishi::types {
    enum class GraphicsAPI : std::uint32_t {
        DirectX11,
        DirectX12,

        OpenGL40,
        OpenGL41,
        OpenGL42,
        OpenGL43,
        OpenGL44,
        OpenGL45,
        OpenGL46,

        Vulkan10,
        Vulkan11,
        Vulkan12,
        Vulkan13,
        Vulkan14,
    };
} // namespace enishi::types