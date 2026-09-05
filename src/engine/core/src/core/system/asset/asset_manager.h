#pragma once
#include "../../errors/errors.h"
#include "../interface_system.h"
#include <assets_system/asset_handle.h>
#include <assets_system/interface_asset_loader.h>
#include <assets_system/interface_asset_system.h>
#include <ecs/registory.h>
#include <engine_types/assets/asset_state.h>
#include <engine_types/assets/model/model_data.h>
#include <engine_types/assets/shader/shader_data.h>
#include <engine_types/assets/texture/texture_data.h>
#include <filesystem>
#include <foundation/option/option.h>
#include <foundation/str/str.h>
#include <foundation/thread/single_thread_executor.h>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>
#include <unordered_map>
#include <vector>

namespace enishi::core {
    class AssetManager : public assets_system::IAssetSystem {
      private:
        using AssetLoader = std::shared_ptr<assets_system::IAssetLoader>;
        using LoaderMap = std::unordered_map<foundation::UTF8, std::vector<AssetLoader>>;

        // IO(ローダー呼び出し)が完了した際にワーカースレッドからメインスレッドへ渡す結果
        // Registoryへの登録はスレッドセーフでないため,
        // 必ずメインスレッド側で行う
        struct CompletedLoad {
            assets_system::AssetHandle handle;
            std::filesystem::path path;
            foundation::Result<assets_system::AssetData, assets_system::AssetError> result;
        };

      private:
        std::thread load_thread;

        ecs::Registory asset_registory;
        std::unordered_map<std::filesystem::path, assets_system::AssetHandle> path_to_handle;
        LoaderMap extension_to_loader;
        std::unordered_map<types::AssetKind, AssetLoader> asset_type_to_loader;

        foundation::SingleThreadExecutor io_executor;
        mutable std::mutex state_mutex;
        std::unordered_map<assets_system::AssetHandle, types::AssetState> asset_states;
        std::mutex completed_loads_mutex;
        std::queue<CompletedLoad> completed_loads;

      public:
        explicit AssetManager(void);

      public:
        // 完了キューを空になるまで処理し, Registoryへの登録まで行う
        void drain_completed_loads(void);

      public:
        foundation::Result<assets_system::AssetHandle, assets_system::AssetError> load_asset(
            const std::filesystem::path& path) noexcept override;

        void release_asset(const assets_system::AssetHandle& handle) noexcept override;

        foundation::Option<const std::filesystem::path&> get_asset_file_name(
            const assets_system::AssetHandle& handle) const noexcept override;

        assets_system::PathObjects find_assets(const std::filesystem::path& target_path,
            const std::unordered_set<std::filesystem::path>& target_extensions)
            const noexcept override;

        foundation::Option<const assets_system::AssetModelData&> get_model_data(
            const assets_system::AssetHandle& handle) const noexcept override;
        foundation::Option<const assets_system::AssetShaderData&> get_shader_data(
            const assets_system::AssetHandle& handle) const noexcept override;
        foundation::Option<const assets_system::AssetTextureData&> get_texture_data(
            const assets_system::AssetHandle& handle) const noexcept override;

        assets_system::PathObjects find_assets(const std::filesystem::path& target_path,
            const types::AssetKind asset_kind) const noexcept override;
        types::AssetState get_asset_state(
            const assets_system::AssetHandle& handle) const noexcept override;
        foundation::UTF8 get_extensions_pattern(
            const types::AssetKind asset_kind) const noexcept override;

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

        template <typename T>
        foundation::Result<types::HandleId, SystemError> register_asset(T&& data) noexcept {
            const auto id = this->asset_registory.create();
            const auto result = this->asset_registory.insert(id, std::move(data));
            if (result.is_err()) {
                this->asset_registory.destroy(id);
                return result.propagation(SystemError::AssetSystemError)
                    .add_message("アセットデータの登録に失敗しました");
            }
            return id;
        }

        // 既に確保済みのEntityIDへアセットデータを挿入する(非同期読み込み完了後の登録用)
        // load_asset内でIOを待たずに発行したハンドルのidをそのまま使うため、
        // 新規にEntityを作る register_asset とは異なりここでは create を呼ばない
        template <typename T>
        foundation::Result<void, SystemError> insert_asset(
            const types::HandleId id, T&& data) noexcept {
            const auto result = this->asset_registory.insert(id, std::forward<T>(data));
            if (result.is_err()) {
                return result.propagation(SystemError::AssetSystemError)
                    .add_message("アセットデータの登録に失敗しました");
            }
            return {};
        }

        [[nodiscard]] foundation::Result<assets_system::AssetHandle, assets_system::AssetError>
        register_model(assets_system::AssetModelData data) noexcept;
        [[nodiscard]] foundation::Result<assets_system::AssetHandle, assets_system::AssetError>
        register_animation(types::ModelData&& data) noexcept;
        [[nodiscard]]
        foundation::Result<assets_system::AssetHandle, assets_system::AssetError> register_shader(
            assets_system::AssetShaderData data) noexcept;
        [[nodiscard]] foundation::Result<assets_system::AssetHandle, assets_system::AssetError>
        register_texture(assets_system::AssetTextureData data) noexcept;
        [[nodiscard]]
        foundation::Result<assets_system::AssetHandle, assets_system::AssetError> register_video(
            assets_system::AssetModelData data) noexcept;
        [[nodiscard]] foundation::Result<assets_system::AssetHandle, assets_system::AssetError>
        register_sound(types::ModelData&& data) noexcept;
        [[nodiscard]] foundation::Result<assets_system::AssetHandle, assets_system::AssetError>
        register_script(types::ModelData&& data) noexcept;

        [[nodiscard]] std::vector<foundation::UTF8> get_extensions(
            const types::AssetKind asset_type) const;

        static std::unordered_set<std::filesystem::path> convert_hash_set(
            const std::vector<foundation::UTF8>& extensions) noexcept;

      private:
        // 指定パスの読み込みタスクをIO専用スレッドへ積む(呼び出し元はブロックしない)
        void request_load(const std::filesystem::path& path,
            const assets_system::AssetHandle& handle,
            const std::vector<AssetLoader>& candidates);

        void set_asset_state(
            const assets_system::AssetHandle& handle, const types::AssetState state) noexcept;

        // IOスレッド側から完了結果をキューへ積む(スレッドセーフ)
        void enqueue_completed_load(CompletedLoad&& completed) noexcept;

        // メインスレッド側で完了結果を1件取り出す(スレッドセーフ)
        [[nodiscard]] foundation::Option<CompletedLoad> dequeue_completed_load(void) noexcept;

        // 読み込み結果1件をRegistoryへ反映し、状態をLoaded/Failedへ確定する(メインスレッド専用)
        void finalize_load(CompletedLoad&& completed) noexcept;
    };
} // namespace enishi::core