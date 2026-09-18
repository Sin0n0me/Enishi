#pragma once
#include <engine_types/assets/asset_data.h>
#include <engine_types/assets/asset_kind.h>
#include <engine_types/assets/asset_state.h>
#include <engine_types/handle/asset/asset_handle.h>
#include <filesystem>
#include <foundation/option/option.h>
#include <foundation/path/path_objects.h>
#include <foundation/result/result.h>
#include <platform/errors/asset_errors.h>
#include <ranges>
#include <regex>
#include <vector>
#include <variant>

namespace enishi::platform {
    class IAssetSystem {
      public:
      public:
        virtual ~IAssetSystem(void) noexcept = default;

        // 任意のアセットを探す場合
        [[nodiscard]] virtual foundation::PathObjects find_assets(
            const std::filesystem::path& target_path,
            const std::unordered_set<std::filesystem::path>& target_extensions) const noexcept = 0;

        [[nodiscard]] virtual foundation::PathObjects find_assets(
            const std::filesystem::path& target_path,
            const types::AssetKind asset_kind) const noexcept = 0;

        // 非同期で読み込みされる
        [[nodiscard]] virtual foundation::Result<types::AssetHandle, AssetError> load_asset(
            const std::filesystem::path& path) noexcept = 0;

        // 指定したハンドルの読み込み状態を取得する
        // ハンドルが存在しない場合はAssetState::NotLoadedを返す
        [[nodiscard]] virtual types::AssetState get_asset_state(
            const types::AssetHandle& handle) const noexcept = 0;

        virtual void release_asset(const types::AssetHandle& handle) noexcept = 0;

        [[nodiscard]] virtual foundation::Option<const std::filesystem::path&> get_asset_file_name(
            const types::AssetHandle& handle) const noexcept = 0;

        [[nodiscard]] virtual foundation::Option<const types::AssetData&> get_asset_data(
            const types::AssetHandle& handle) const noexcept = 0;

        template <typename T>
        [[nodiscard]] foundation::Option<const T&> get_asset(
            const types::AssetHandle& handle) const noexcept {
            const auto asset_data = this->get_asset_data(handle);
            if (asset_data.is_none()) {
                return {};
            }

            const auto* const data = std::get_if<T>(&asset_data.unwrap());
            if (!data) {
                return {};
            }
            return *data;
        }

        [[nodiscard]] virtual foundation::UTF8 get_extensions_pattern(
            const types::AssetKind asset_kind) const noexcept = 0;
    };
} // namespace enishi::platform
