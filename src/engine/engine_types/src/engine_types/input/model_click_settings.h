#pragma once
#include <engine_types/physics/rigid_body/rigid_body_kind.h>
#include <vector>
#include <string>
#include <cstdint>

namespace enishi::types {
    enum class ModelClickMode { None, Touch, Push };

    struct ModelClickSettings {
        ModelClickMode mode = ModelClickMode::Push;
        std::vector<types::RigidBodyKind> push_states{types::RigidBodyKind::Kinematic,
            types::RigidBodyKind::Dynamic,
            types::RigidBodyKind::DynamicAdjustBone};
        std::uint64_t click_count = 0;
        std::string last_target;
        std::string last_result;
        [[nodiscard]] bool accepts_push(types::RigidBodyKind kind) const;
    };
} // namespace enishi::types
