#pragma once
#include "../../errors/errors.h"
#include <assets_system/asset_load_scheduler.h>
#include <assets_system/interface_asset_loader.h>
#include <ecs/registry.h>
#include <engine_types/assets/asset_state.h>
#include <engine_types/assets/model/model_data.h>
#include <engine_types/assets/shader/shader_data.h>
#include <engine_types/assets/texture/texture_data.h>
#include <engine_types/handle/asset/asset_handle.h>
#include <filesystem>
#include <foundation/option/option.h>
#include <foundation/str/str.h>
#include <memory>
#include <mutex>
#include <platform/asset/interface_asset_system.h>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace enishi::core {
    class AssetManager : public platform::IAssetSystem {
      private:
        using AssetLoader = std::shared_ptr<assets_system::IAssetLoader>;
        using LoaderMap = std::unordered_map<foundation::UTF8, std::vector<AssetLoader>>;
        using CompletedLoad = assets_system::AssetLoadScheduler::CompletedLoad;

      private:
        // 以下はいずれも「一度読み込んだ結果を保持しておくキャッシュ」に過ぎず
        // 外部から見た本クラスの振る舞い(同じハンドルには同じデータを返す)は変化しない
        // そのため、データ取得系のAPI(get_model_data等)をconstに保ったまま
        // 遅延読み込み(ensure_asset_loaded)からも書き込めるようにmutableにしている
        mutable ecs::Registry asset_registry;
        mutable std::unordered_map<std::filesystem::path, types::AssetHandle> path_to_handle;
        mutable std::mutex state_mutex;
        mutable std::unordered_map<types::AssetHandle, types::AssetState> asset_states;
        mutable assets_system::AssetLoadScheduler load_scheduler;

        LoaderMap extension_to_loader;
        std::unordered_map<types::AssetKind, AssetLoader> asset_type_to_loader;

      public:
        explicit AssetManager(void);

        foundation::Result<types::AssetHandle, platform::AssetError> load_asset(
            const std::filesystem::path& path) noexcept override;

        void release_asset(const types::AssetHandle& handle) noexcept override;

        [[nodiscard]] foundation::Option<const std::filesystem::path&> get_asset_file_name(
            const types::AssetHandle& handle) const noexcept override;

        [[nodiscard]] foundation::PathObjects find_assets(const std::filesystem::path& target_path,
            const std::unordered_set<std::filesystem::path>& target_extensions)
            const noexcept override;
        [[nodiscard]] foundation::PathObjects find_assets(const std::filesystem::path& target_path,
            const types::AssetKind asset_kind) const noexcept override;

        [[nodiscard]] foundation::Option<const types::AssetModelData&> get_model_data(
            const types::AssetHandle& handle) const noexcept override;
        [[nodiscard]] foundation::Option<const types::AssetShaderData&> get_shader_data(
            const types::AssetHandle& handle) const noexcept override;
        [[nodiscard]] foundation::Option<const types::AssetTextureData&> get_texture_data(
            const types::AssetHandle& handle) const noexcept override;

        [[nodiscard]] types::AssetState get_asset_state(
            const types::AssetHandle& handle) const noexcept override;
        [[nodiscard]] foundation::UTF8 get_extensions_pattern(
            const types::AssetKind asset_kind) const noexcept override;

      public:
        // 完了済みの結果を取れるだけ取り出してRegistryへ反映する(非ブロッキング)
        // 定期的に呼ばなくても正しく動く
        void drain_completed_loads(void) const;

      private:
        template <typename T, typename... Args>
        std::shared_ptr<T> add_loader(const types::AssetKind asset_type, Args&&... args) {
            auto loader = std::make_shared<T>(args...);
            for (const auto& extension : loader->get_supported_extension()) {
                this->extension_to_loader[extension].emplace_back(loader);
            }
            this->asset_type_to_loader[asset_type] = loader;
            return loader;
        }

        // 既に確保済みのEntityIDへアセットデータを挿入する(非同期読み込み完了後の登録用)
        // load_asset内でIOを待たずに発行したハンドルのidをそのまま使うため、
        // 新規にEntityを作るのではなくここではinsertのみ行う
        // asset_registryがmutableのため、constメソッドの中からも呼び出せる
        template <typename T>
        foundation::Result<void, SystemError> insert_asset(
            const types::HandleId id, T&& data) const noexcept {
            const auto result = this->asset_registry.insert(id, std::forward<T>(data));
            if (result.is_err()) {
                return result.propagation(SystemError::AssetSystemError)
                    .add_message("アセットデータの登録に失敗しました");
            }
            return {};
        }

        [[nodiscard]] std::vector<foundation::UTF8> get_extensions(
            const types::AssetKind asset_type) const;

        static std::unordered_set<std::filesystem::path> convert_hash_set(
            const std::vector<foundation::UTF8>& extensions) noexcept;

      private:
        // 非同期読み込み関連

        // 指定パスの読み込みジョブをスケジューラへ積む(呼び出し元はブロックしない)
        void request_load(const std::filesystem::path& path,
            const types::AssetHandle& handle,
            const std::vector<AssetLoader>& candidates) const;

        void set_asset_state(
            const types::AssetHandle& handle, const types::AssetState state) const noexcept;

        // スケジューラの完了結果をRegistryへ反映
        void commit_completed_load(CompletedLoad&& completed) const noexcept;

        // データ取得要求が来た時点で該当ハンドルが読み込み中(Queued/Loading)なら
        // IO完了とRegistryへの反映が終わるまで呼び出し元をブロックする
        void ensure_asset_loaded(const types::AssetHandle& handle) const noexcept;
    };
} // namespace enishi::core
