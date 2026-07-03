#pragma once
#include "asset_handle.h"
#include "asset_type.h"
#include "errors/errors.h"
#include <engine_types/assets/model/model_data.h>
#include <engine_types/assets/shader/shader_data.h>
#include <engine_types/assets/texture/texture_data.h>
#include <filesystem>
#include <foundation/option/option.h>
#include <foundation/result/result.h>
#include <unordered_set>
#include <vector>

namespace enishi::assets_system {
    class IAssetSystem {
      public:
        virtual ~IAssetSystem(void) noexcept = default;

        // 任意のアセットを探す場合
        [[nodiscard]] virtual std::vector<std::filesystem::path> find_assets(
            const std::filesystem::path& target_path,
            const std::unordered_set<std::filesystem::path>& target_extensions) const noexcept = 0;

        [[nodiscard]] virtual std::vector<std::filesystem::path> find_models(
            const std::filesystem::path& target_path) const noexcept = 0;

        [[nodiscard]] virtual std::vector<std::filesystem::path> find_shaders(
            const std::filesystem::path& target_path) const noexcept = 0;

        [[nodiscard]] virtual std::vector<std::filesystem::path> find_textures(
            const std::filesystem::path& target_path) const noexcept = 0;

        [[nodiscard]] virtual std::vector<std::filesystem::path> find_scripts(
            const std::filesystem::path& target_path) const noexcept = 0;

        [[nodiscard]] virtual foundation::Result<AssetHandle, AssetError> load_asset(
            const std::filesystem::path& path) noexcept = 0;

        virtual void release_asset(const AssetHandle& handle) noexcept = 0;

        [[nodiscard]] virtual foundation::Option<const std::filesystem::path&> get_asset_file_name(
            const AssetHandle& handle) const noexcept = 0;

        [[nodiscard]] virtual foundation::Option<const types::ModelData&> get_model_data(
            const AssetHandle& handle) const noexcept = 0;

        [[nodiscard]] virtual foundation::Option<const types::ShaderData&> get_shader_data(
            const AssetHandle& handle) const noexcept = 0;

        [[nodiscard]] virtual foundation::Option<const types::TextureData&> get_texture_data(
            const AssetHandle& handle) const noexcept = 0;
    };
} // namespace enishi::assets_system
