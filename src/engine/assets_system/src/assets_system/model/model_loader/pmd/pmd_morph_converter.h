#pragma once
#include "pmd_file_struct.h"
#include <assets_system/errors/errors.h>
#include <engine_types/assets/model/addons/morph.h>
#include <engine_types/assets/model/addons/morph_target.h>
#include <span>

namespace enishi::assets_system {
    struct PMDMorphAddons {
        types::AddonMorphs legacy;
        types::AddonMorphTargets targets;
    };

    [[nodiscard]] foundation::Result<PMDMorphAddons, AssetError> convert_pmd_morphs(
        std::span<const PMDMorph> morphs, std::size_t vertex_count);
} // namespace enishi::assets_system
