#pragma once
#include "../handle/handle_type.h"
#include <cstdint>

namespace enishi::types {
    // バッファーの種類などはHandleIDから判断する
    // あくまで何かを示すだけ
    enum class RenderHandleType : std::uint32_t {
        Mesh,
        Texture,
        Shader,
        Buffer,
        View,
        Rasterizer,
        ViewPort,
        Topology,
        InputLayout,
        Draw,
    };

    struct RenderHandle {
        HandleId id;
        RenderHandleType type;

        bool is_valid(void) const noexcept;
    };

    constexpr static RenderHandle DEFAULT_RENDER_TARGET = RenderHandle{
        .id = INVALID_HANDLE_ID,
        .type = RenderHandleType::View,
    };
} // namespace enishi::types
