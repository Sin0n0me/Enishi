#pragma once
#include "camera.h"
#include "shadow.h"

namespace enishi::types {
    struct ShadowReceiverProjection {
        UniformCamera camera;
        UniformShadow shadow;
    };
    ShadowReceiverProjection fit_shadow_receiver(
        const ShadowBounds& world_bounds, ShadowBounds light_bounds, const UniformCamera& camera);
} // namespace enishi::types
