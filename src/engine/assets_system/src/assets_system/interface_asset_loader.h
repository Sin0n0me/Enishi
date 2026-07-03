#pragma once
#include "asset_data.h"
#include "asset_type.h"
#include "errors/errors.h"
#include <filesystem>
#include <foundation/result/result.h>
#include <foundation/str/str.h>
#include <vector>

namespace enishi::assets_system {
    class IAssetLoader {
      public:
        virtual ~IAssetLoader(void) noexcept = default;

        [[nodiscard]] virtual foundation::Result<AssetData, AssetError> load(
            const std::filesystem::path& path) noexcept = 0;

        [[nodiscard]] virtual std::vector<foundation::UTF8> get_supported_extension(
            void) const noexcept = 0;

        [[nodiscard]] virtual AssetType get_target_asset_type(void) const noexcept = 0;
    };
} // namespace enishi::assets_system