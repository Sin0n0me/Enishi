#pragma once
#include <engine_types/handle/physics/handles/model_handles.h>
#include <vector>

namespace enishi::types {
    struct PhysicsObject {
        std::vector<PhysicsModelHandles> model_parts;
    };
} // namespace enishi::types