#pragma once
#include "../../errors/errors.h"
#include "../../interface_asset_loader.h"
#include "../../interface_asset_system.h"
#include "../../texture/texture_loader.h"
#include "interface_model_loader.h"
#include "pmd/pmd_data.h"
#include <engine_types/assets/model/model_data.h>
#include <filesystem>
#include <foundation/result/result.h>
#include <vector>

namespace enishi::assets_system {
    class ModelLoader : public IAssetLoader {
      private:
        std::shared_ptr<TextureLoader> texture_loader;
        std::unordered_map<foundation::UTF8, std::unique_ptr<IModelLoader>> loaders;

      public:
        ModelLoader(std::shared_ptr<TextureLoader> texture_loader);

        foundation::Result<AssetData, AssetError> load(
            const std::filesystem::path& path) noexcept override;
        std::vector<foundation::UTF8> get_supported_extension(void) const noexcept override;

      private:
        static types::ModelData to_model_data_from_pmd(const PMDData& data);

        types::AssetKind get_target_asset_type(void) const noexcept override;
    };
} // namespace enishi::assets_system