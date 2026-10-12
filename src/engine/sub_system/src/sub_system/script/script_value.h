#pragma once
#include <cstdint>
#include <string>
#include <variant>
#include <vector>

namespace enishi::sub_system {
    enum class ScriptValueKind : std::uint32_t {
        Unit,
        Boolean,
        SignedInteger,
        UnsignedInteger,
        Float,
        String,
        Tuple,
        Array,
        Vector,
        Matrix,
        Struct,
        Enum,
        Range
    };

    // Values own their data and remain valid after the Musubu execution has been destroyed.
    // Matrix children are columns; struct/enum children carry their field names.
    struct ScriptValue {
        ScriptValueKind kind = ScriptValueKind::Unit;
        std::string type_name;
        std::string name;
        std::variant<std::monostate, bool, std::int64_t, std::uint64_t, double, std::string> scalar;
        std::string variant_name;
        std::vector<ScriptValue> children;
    };
} // namespace enishi::sub_system
