#pragma once
#include <animation/cursor_pose.h>
#include <platform/window/interface_window.h>

namespace enishi::core {
    [[nodiscard]] std::optional<animation::CursorRay> read_cursor_ray(
        const platform::IWindow& window, const types::UniformCamera& camera,
        bool allow_outside = false);
} // namespace enishi::core
