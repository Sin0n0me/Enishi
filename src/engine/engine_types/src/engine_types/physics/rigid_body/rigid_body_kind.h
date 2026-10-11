#pragma once
#include <cstdint>

namespace enishi::types {
    enum class RigidBodyKind : std::uint8_t {
        Kinematic,         // ボーン追従
        Dynamic,           //
        DynamicAdjustBone, //
    };
} // namespace enishi::types
