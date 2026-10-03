#pragma once
#include <engine_types/handle/physics/physics_handle.h>
#include <vector>

namespace enishi::types {
    struct PhysicsModelHandles {
        PhysicsHandle shape_handle;
        PhysicsHandle motion_state_handle;
        PhysicsHandle rigid_body_handle;
    };
} // namespace enishi::types