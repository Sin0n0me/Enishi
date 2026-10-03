#pragma once
#include <cstdint>

namespace enishi::types {
    enum class AssetKind : std::uint8_t {
        Unknown = 0,
        Animation,
        Model,
        Shader,
        Audio,
        Texture,
        Script,
        Video,
    };
}