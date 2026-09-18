#pragma once
#include "../handle/handle_type.h"
#include <cstdint>

namespace enishi::types {
    enum class GuiBackend : std::uint32_t {
        Type,
        Function,
    };

    struct ScriptHandle {
        HandleId id;
        GuiBackend type;

        bool is_valid(void) const noexcept;
    };
} // namespace enishi::types