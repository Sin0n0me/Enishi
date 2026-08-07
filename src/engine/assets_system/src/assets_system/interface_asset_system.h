#pragma once
#include "asset_data.h"
#include "asset_handle.h"
#include "asset_type.h"
#include "errors/errors.h"
#include "utility/path_objects.h"
#include <filesystem>
#include <foundation/option/option.h>
#include <foundation/result/result.h>
#include <ranges>
#include <regex>
#include <vector>

namespace enishi::assets_system {
    class IAssetSystem {
      public:
        virtual ~IAssetSystem(void) noexcept = default;

        // 任意のアセットを探す場合
        [[nodiscard]] virtual PathObjects find_assets(const std::filesystem::path& target_path,
            const std::unordered_set<std::filesystem::path>& target_extensions) const noexcept = 0;

        [[nodiscard]] virtual PathObjects find_models(
            const std::filesystem::path& target_path) const noexcept = 0;

        [[nodiscard]] virtual PathObjects find_shaders(
            const std::filesystem::path& target_path) const noexcept = 0;

        [[nodiscard]] virtual PathObjects find_textures(
            const std::filesystem::path& target_path) const noexcept = 0;

        [[nodiscard]] virtual PathObjects find_scripts(
            const std::filesystem::path& target_path) const noexcept = 0;

        [[nodiscard]] virtual foundation::Result<AssetHandle, AssetError> load_asset(
            const std::filesystem::path& path) noexcept = 0;

        virtual void release_asset(const AssetHandle& handle) noexcept = 0;

        [[nodiscard]] virtual foundation::Option<const std::filesystem::path&> get_asset_file_name(
            const AssetHandle& handle) const noexcept = 0;

        [[nodiscard]] virtual foundation::Option<const AssetModelData&> get_model_data(
            const AssetHandle& handle) const noexcept = 0;

        [[nodiscard]] virtual foundation::Option<const AssetShaderData&> get_shader_data(
            const AssetHandle& handle) const noexcept = 0;

        [[nodiscard]] virtual foundation::Option<const AssetTextureData&> get_texture_data(
            const AssetHandle& handle) const noexcept = 0;

        [[nodiscard]] virtual foundation::UTF8 model_extensions_pattern(void) const noexcept = 0;

        [[nodiscard]] virtual foundation::UTF8 shader_extensions_pattern(void) const noexcept = 0;

        [[nodiscard]] virtual foundation::UTF8 texture_extensions_pattern(void) const noexcept = 0;

        [[nodiscard]] virtual foundation::UTF8 script_extensions_pattern(void) const noexcept = 0;
    };
} // namespace enishi::assets_system
