#pragma once
#include "../window/window_types.h"
#include <cstdint>

namespace enishi::types {
    struct ClientMousePosition {
        std::int32_t x;
        std::int32_t y;
    };

    struct ScreenMousePosition {
        std::int32_t x;
        std::int32_t y;
    };

    struct MouseWheel {
        std::int32_t x;
        std::int32_t y;
    };

    constexpr ScreenMousePosition to_screen(
        const ClientMousePosition& mouse_position, const WindowPosition& window_position) noexcept {
        return ScreenMousePosition{
            .x = window_position.x + mouse_position.x,
            .y = window_position.y + mouse_position.y,
        };
    }

    constexpr ClientMousePosition to_client(
        const ScreenMousePosition& mouse_position, const WindowPosition& window_position) noexcept {
        return ClientMousePosition{
            .x = window_position.x - mouse_position.x,
            .y = window_position.y - mouse_position.y,
        };
    }
} // namespace enishi::types