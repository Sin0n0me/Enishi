#pragma once
#include "vrm_context.h"

namespace enishi::assets_system {
    bool bind_vrm_texture_transforms(
        VRMContext& context, types::MorphTarget& target, const VRMJson& expression);
}
