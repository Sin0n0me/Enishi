#pragma once
#include "../errors/errors.h"
#include "../interface_asset_loader.h"
#include <cstdint>
#include <engine_types/assets/texture/texture_data.h>
#include <filesystem>
#include <foundation/result/result.h>
#include <vector>

namespace enishi::assets_system {
    class TextureLoader : public IAssetLoader {
      private:
      public:
        foundation::Result<AssetData, AssetError> load(
            const std::filesystem::path& path) noexcept override;
        std::vector<foundation::UTF8> get_supported_extension(void) const noexcept override;

        static void generate_mip_chain(types::TextureData& data);
    };
} // namespace enishi::assets_system