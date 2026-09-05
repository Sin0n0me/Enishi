#pragma once
#include "asset_data.h"
#include "asset_handle.h"
#include "errors/errors.h"
#include "utility/path_objects.h"
#include <engine_types/assets/asset_kind.h>
#include <engine_types/assets/asset_state.h>
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

        [[nodiscard]] virtual PathObjects find_assets(const std::filesystem::path& target_path,
            const types::AssetKind asset_kind) const noexcept = 0;

        // 非同期で読み込みされる
        [[nodiscard]] virtual foundation::Result<AssetHandle, AssetError> load_asset(
            const std::filesystem::path& path) noexcept = 0;

        // 指定したハンドルの読み込み状態を取得する
        // ハンドルが存在しない場合はAssetState::NotLoadedを返す
        [[nodiscard]] virtual types::AssetState get_asset_state(
            const AssetHandle& handle) const noexcept = 0;

        virtual void release_asset(const AssetHandle& handle) noexcept = 0;

        [[nodiscard]] virtual foundation::Option<const std::filesystem::path&> get_asset_file_name(
            const AssetHandle& handle) const noexcept = 0;

        [[nodiscard]] virtual foundation::Option<const AssetModelData&> get_model_data(
            const AssetHandle& handle) const noexcept = 0;

        [[nodiscard]] virtual foundation::Option<const AssetShaderData&> get_shader_data(
            const AssetHandle& handle) const noexcept = 0;

        [[nodiscard]] virtual foundation::Option<const AssetTextureData&> get_texture_data(
            const AssetHandle& handle) const noexcept = 0;

        [[nodiscard]] virtual foundation::UTF8 get_extensions_pattern(
            const types::AssetKind asset_kind) const noexcept = 0;
    };
} // namespace enishi::assets_system
