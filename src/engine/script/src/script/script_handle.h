#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace script {
    enum class HandleType : std::uint8_t {};

    struct ScriptHandle {
        std::uint32_t id;
    };

    struct TypeHandle {
        std::uint32_t id;
    };
} // namespace script
