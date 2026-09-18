#pragma once
#include <cstdint>

namespace enishi::types {
    enum class FillMode : std::uint8_t {
        Solid,     // 塗りつぶし
        Wireframe, // ワイヤーフレーム
    };

    enum class CullMode : std::uint8_t {
        None,  // カリングなし
        Front, // 前面カリング
        Back,  // 背面カリング
    };

    enum class FrontFace : std::uint8_t {
        CounterClockwise, // 反時計回りが表(OpenGL, Vulkan標準)
        Clockwise,        // 時計回りが表(DirectX標準)
    };
} // namespace enishi::types