#pragma once
#include "collision.h"
#include <engine_types/input/input_types.h>
#include <optional>

namespace enishi::types {
    struct ModelHit {
        CollisionModelId model;
        BoneIndex bone;
        glm::vec3 position;
        float distance;
    };

    struct ModelClick {
        ClientMousePosition cursor;
        std::optional<ModelHit> hit;
    };
} // namespace enishi::types
