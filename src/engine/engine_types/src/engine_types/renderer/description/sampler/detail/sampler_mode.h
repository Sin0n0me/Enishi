#pragma once
#include <cstdint>

namespace enishi::types {
    enum class FilterMode : std::uint8_t {
        Nearest, // ニアレスト
        Linear,  // リニア
    };

    enum class AddressMode : std::uint8_t {
        Repeat, // タイリング
        Clamp,  // 端をクランプ
        Mirror, // ミラーリング
    };

    enum class AnisotropyLevel : std::uint8_t {
        None = 1,
        X2 = 2,
        X4 = 4,
        X8 = 8,
        X16 = 16,
    };
} // namespace enishi::types