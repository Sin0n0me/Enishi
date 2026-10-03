#pragma once
#include "../handle/handle_type.h"
#include <cstdint>

namespace enishi::types {
    enum class GuiBackend : std::uint32_t {
        Win32,
        Wayland,
        X11,
    };

    struct SystemHandle {
        HandleId id;
        GuiBackend type;

        bool is_valid(void) const noexcept;
    };
} // namespace enishi::types