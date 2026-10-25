#pragma once
#include "vrm_context.h"

namespace enishi::assets_system {
    bool make_vrm_material(VRMContext& context, const cgltf_material* material,
        std::uint32_t first, std::uint32_t count);
}
