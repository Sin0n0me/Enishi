#pragma once
#include "../errors/errors.h"
#include "../interface_asset_loader.h"
#include "../utility/binary_reader.h"
#include <engine_types/assets/shader/shader_data.h>
#include <filesystem>
#include <foundation/option/option.h>
#include <foundation/result/result.h>
#include <foundation/str/str.h>
#include <unordered_map>

namespace enishi::assets_system {
    class ShaderLoader : public IAssetLoader {
      private:
        std::unordered_map<foundation::UTF8, types::ShaderBinaryType> supported_extension;

      public:
        ShaderLoader(void);

        foundation::Result<AssetData, AssetError> load(
            const std::filesystem::path& path) noexcept override;
        std::vector<foundation::UTF8> get_supported_extension(void) const noexcept override;

      private:
        [[nodiscard]]
        static foundation::Result<types::ShaderData, AssetError> load_spir_v(
            BinaryReader& reader) noexcept;

        [[nodiscard]]
        static foundation::Result<types::ShaderData, AssetError> load_dxbc(
            BinaryReader& reader) noexcept;

        AssetType get_target_asset_type(void) const noexcept override;
    };
} // namespace enishi::assets_system
