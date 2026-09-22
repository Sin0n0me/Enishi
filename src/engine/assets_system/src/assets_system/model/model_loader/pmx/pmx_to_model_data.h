#pragma once
#include "../../../interface_asset_loader.h"
#include "pmx_data.h"

namespace enishi::assets_system {
    class PMXToModelData {
      public:
        [[nodiscard]] static foundation::Result<types::AssetModelData, AssetError> to_model_data(
            const std::filesystem::path& path, const PMXData& data, IAssetLoader* texture_loader);
    };
} // namespace enishi::assets_system
