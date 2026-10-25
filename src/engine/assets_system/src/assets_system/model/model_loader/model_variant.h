#pragma once
#include "pmd/pmd_data.h"
#include "pmx/pmx_data.h"
#include <engine_types/assets/asset_data.h>
#include <foundation/option/option.h>
#include <variant>

namespace enishi::assets_system {
    using ModelVariant = std::variant<std::monostate,
        std::unique_ptr<PMDData>,
        std::unique_ptr<PMXData>,
        types::AssetModelData>;
} // namespace enishi::assets_system
