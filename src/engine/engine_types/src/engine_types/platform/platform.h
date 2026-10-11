#pragma once
#include <cstdint>

namespace enishi::types {
    enum class Platform : std::uint32_t {
        Windows,
        X11,
        Wayland,
    };
}