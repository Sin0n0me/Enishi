#include "asset_manager.h"
#include <assets_system/model/model_loader/model_loader.h>
#include <assets_system/shader/shader_loader.h>
#include <assets_system/texture/texture_loader.h>
#include <foundation/log/logger.h>

namespace enishi::core {
    foundation::UTF8 make_extension_regex(const std::vector<foundation::UTF8>& extensions) {
        constexpr std::string_view REGEX_PREFIX = "(";
        constexpr std::string_view REGEX_SUFFIX = ")$";

        // `.` + 1文字以上という前提
        foundation::UTF8 pattern(REGEX_PREFIX);
        for (std::size_t i = 0; i < extensions.size(); ++i) {
            if (i != 0) {
                pattern += '|';
            }
            pattern += "\\"; // 先頭の`.`のみエスケープ
            pattern += extensions[i];
        }
        pattern += REGEX_SUFFIX;

        return pattern;
    }

    AssetManager::AssetManager(void) {
        this->add_loader<assets_system::ShaderLoader>(types::AssetKind::Shader);
        auto texture_loader =
            this->add_loader<assets_system::TextureLoader>(types::AssetKind::Texture);
        this->add_loader<assets_system::ModelLoader>(types::AssetKind::Model, texture_loader);
    }

    foundation::Result<assets_system::AssetHandle, assets_system::AssetError>
    AssetManager::load_asset(const std::filesystem::path& path) noexcept {
        // すでにAssetを保持している場合はそのまま保管しているハンドルを返す
        const auto normalized_path = path.lexically_normal();
        const auto iter = this->path_to_handle.find(normalized_path);
        if (iter != this->path_to_handle.end()) {
            return iter->second;
        }

        if (!path.has_extension()) {
            return foundation::Error(assets_system::AssetError::NotFound);
        }

        // 拡張子に応じたアセットローダーを探す
        const auto extention = path.extension();
        const auto asset_iter = this->extension_to_loader.find(extention.string<char>());
        if (asset_iter == this->extension_to_loader.end()) {
            return foundation::Error(assets_system::AssetError::NotFound,
                std::format("not found loader. target: {}", path.string<char>()));
        }

        const auto& candidates = asset_iter->second;
        if (candidates.empty()) {
            return foundation::Error(assets_system::AssetError::NotFound);
        }

        // ハンドルはIOの完了を待たずにこの場で発行する
        // (typeは最有力候補である先頭ローダーの対応アセット種別を暫定的に採用する。
        //  1拡張子に複数ローダーが対応するケースは稀であり、通常はここで確定する)
        const auto handle = assets_system::AssetHandle{
            .id = this->asset_registory.create(),
            .type = candidates.front()->get_target_asset_type(),
        };

        this->path_to_handle[normalized_path] = handle;
        this->set_asset_state(handle, types::AssetState::Queued);
        this->request_load(normalized_path, handle, candidates);

        return handle;
    }

    void AssetManager::release_asset(const assets_system::AssetHandle& handle) noexcept {
    }

    foundation::Option<const std::filesystem::path&> AssetManager::get_asset_file_name(
        const assets_system::AssetHandle& handle) const noexcept {
        return foundation::Option<const std::filesystem::path&>();
    }

    assets_system::PathObjects AssetManager::find_assets(const std::filesystem::path& target_path,
        const std::unordered_set<std::filesystem::path>& target_extensions) const noexcept {
        assets_system::PathObjects matched_files;
        std::vector<std::filesystem::path> directory_stack;
        std::error_code ec;

        // 初期ディレクトリの存在, 走査確認
        std::filesystem::path root_path(target_path);
        if (!std::filesystem::is_directory(root_path, ec) || ec) {
            foundation::Logger::warning(
                std::format("not a directory. find path: {}", target_path.string<char>()));

            return matched_files;
        }

        // 再帰的に探索
        directory_stack.push_back(root_path);
        while (!directory_stack.empty()) {
            const std::filesystem::path current_dir = directory_stack.back();
            directory_stack.pop_back();

            auto it = std::filesystem::directory_iterator(current_dir, ec);
            if (ec) {
                foundation::Logger::error(
                    std::format("access error. find path: {}", current_dir.string<char>()));
                continue; // アクセス権限エラーなどはスキップ
            }

            for (const auto& entry : it) {
                const std::filesystem::file_status status = entry.status(ec);
                if (ec) {
                    continue;
                }

                if (std::filesystem::is_directory(status)) {
                    directory_stack.push_back(entry.path());
                } else if (std::filesystem::is_regular_file(status)) {
                    if (target_extensions.contains(entry.path().extension())) {
                        matched_files.add(entry.path());
                    }
                }
            }
        }

        return matched_files;
    }

    void AssetManager::request_load(const std::filesystem::path& path,
        const assets_system::AssetHandle& handle,
        const std::vector<AssetLoader>& candidates) {
        // candidatesはthis->extension_to_loaderが保持するvectorへの参照であり、
        // ワーカースレッド上で安全に使えるようコピーしてキャプチャする(shared_ptrのコピーなので軽量)
        this->io_executor.submit([this, path, handle, candidates] {
            this->set_asset_state(handle, types::AssetState::Loading);

            // 1つの拡張子が複数ローダーに対応している場合、最初に正常に読み込めた結果を採用する
            for (const auto& loader : candidates) {
                auto result = loader->load(path);
                if (result.is_ok()) {
                    this->enqueue_completed_load(CompletedLoad{
                        .handle = handle,
                        .path = path,
                        .result = std::move(result),
                    });
                    return;
                }
            }

            this->enqueue_completed_load(CompletedLoad{
                .handle = handle,
                .path = path,
                .result = foundation::Error(assets_system::AssetError::NotFound,
                    std::format("読み込みに失敗しました. target: {}", path.string<char>())),
            });
        });
    }

    void AssetManager::set_asset_state(
        const assets_system::AssetHandle& handle, const types::AssetState state) noexcept {
        const std::lock_guard<std::mutex> lock(this->state_mutex);
        this->asset_states[handle] = state;
    }

    void AssetManager::enqueue_completed_load(CompletedLoad&& completed) noexcept {
        {
            const std::lock_guard<std::mutex> lock(this->completed_loads_mutex);
            this->completed_loads.push(std::move(completed));
        }
    }

    foundation::Option<AssetManager::CompletedLoad> AssetManager::dequeue_completed_load(
        void) noexcept {
        const std::lock_guard<std::mutex> lock(this->completed_loads_mutex);
        if (this->completed_loads.empty()) {
            return {};
        }

        auto&& completed = std::move(this->completed_loads.front());
        this->completed_loads.pop();
        return completed;
    }

    void AssetManager::drain_completed_loads(void) {
        for (;;) {
            auto completed = this->dequeue_completed_load();
            if (completed.is_none()) {
                break;
            }

            this->finalize_load(std::move(completed).unwrap_mut());
        }
    }

    void AssetManager::finalize_load(CompletedLoad&& completed) noexcept {
        if (completed.result.is_err()) {
            foundation::Logger::warning(
                std::format("アセットの読み込みに失敗しました. path: {}, message: {}",
                    completed.path.string<char>(),
                    completed.result.unwrap_err().get_message()));

            this->path_to_handle.erase(completed.path);
            this->asset_registory.destroy(completed.handle.id);
            this->set_asset_state(completed.handle, types::AssetState::Failed);
            return;
        }

        // Registoryへの実データ挿入はメインスレッドでのみ行う(ComponentPoolはスレッドセーフでないため)
        auto&& asset_data = std::move(completed.result).unwrap_mut();
        const auto insert_result = std::visit(
            [this, &completed](
                auto&& data) { return this->insert_asset(completed.handle.id, std::move(data)); },
            std::move(asset_data));

        if (insert_result.is_err()) {
            foundation::Logger::error(insert_result.unwrap_err().get_message());

            this->path_to_handle.erase(completed.path);
            this->asset_registory.destroy(completed.handle.id);
            this->set_asset_state(completed.handle, types::AssetState::Failed);
            return;
        }

        this->set_asset_state(completed.handle, types::AssetState::Loaded);
    }

    assets_system::PathObjects AssetManager::find_assets(const std::filesystem::path& target_path,
        const types::AssetKind asset_kind) const noexcept {
        const auto& extensions = this->get_extensions(asset_kind);
        return this->find_assets(target_path, AssetManager::convert_hash_set(extensions));
    }

    types::AssetState enishi::core::AssetManager::get_asset_state(
        const assets_system::AssetHandle& handle) const noexcept {
        const std::lock_guard<std::mutex> lock(this->state_mutex);

        const auto iter = this->asset_states.find(handle);
        if (iter == this->asset_states.end()) {
            return types::AssetState::NotLoaded;
        }
        return iter->second;
    }

    foundation::UTF8 enishi::core::AssetManager::get_extensions_pattern(
        const types::AssetKind asset_kind) const noexcept {
        const auto& extensions = this->get_extensions(asset_kind);
        return make_extension_regex(extensions);
    }

    foundation::Option<const assets_system::AssetModelData&> core::AssetManager::get_model_data(
        const assets_system::AssetHandle& handle) const noexcept {
        return this->asset_registory.get<assets_system::AssetModelData>(handle.id);
    }

    foundation::Option<const assets_system::AssetShaderData&> AssetManager::get_shader_data(
        const assets_system::AssetHandle& handle) const noexcept {
        return this->asset_registory.get<assets_system::AssetShaderData>(handle.id);
    }

    foundation::Option<const assets_system::AssetTextureData&> AssetManager::get_texture_data(
        const assets_system::AssetHandle& handle) const noexcept {
        return this->asset_registory.get<assets_system::AssetTextureData>(handle.id);
    }

    foundation::Result<assets_system::AssetHandle, assets_system::AssetError>
    core::AssetManager::register_model(assets_system::AssetModelData data) noexcept {
        const auto asset_id = this->register_asset(std::move(data));
        if (asset_id.is_err()) {
            return asset_id.propagation(assets_system::AssetError::AlreadyHasAsset)
                .add_message("モデルの追加に失敗しました");
        }
        return assets_system::AssetHandle{
            .id = asset_id.unwrap(),
            .type = types::AssetKind::Model,
        };
    }

    foundation::Result<assets_system::AssetHandle, assets_system::AssetError>
    core::AssetManager::register_shader(assets_system::AssetShaderData data) noexcept {
        const auto asset_id = this->register_asset(std::move(data));
        if (asset_id.is_err()) {
            return asset_id.unwrap_err()
                .propagation(assets_system::AssetError::AlreadyHasAsset)
                .add_message("シェーダーの追加に失敗しました");
        }
        return assets_system::AssetHandle{
            .id = asset_id.unwrap(),
            .type = types::AssetKind::Shader,
        };
    }

    foundation::Result<assets_system::AssetHandle, assets_system::AssetError>
    core::AssetManager::register_texture(assets_system::AssetTextureData data) noexcept {
        const auto asset_id = this->register_asset(std::move(data));
        if (asset_id.is_err()) {
            return asset_id.propagation(assets_system::AssetError::AlreadyHasAsset)
                .add_message("テクスチャの追加に失敗しました");
        }
        return assets_system::AssetHandle{
            .id = asset_id.unwrap(),
            .type = types::AssetKind::Texture,
        };
    }

    std::vector<foundation::UTF8> core::AssetManager::get_extensions(
        const types::AssetKind asset_kind) const {
        const auto iter = this->asset_type_to_loader.find(asset_kind);
        if (iter == this->asset_type_to_loader.end()) {
            return {};
        }

        return iter->second->get_supported_extension();
    }

    std::unordered_set<std::filesystem::path> AssetManager::convert_hash_set(
        const std::vector<foundation::UTF8>& extensions) noexcept {
        std::unordered_set<std::filesystem::path> path_set;
        path_set.reserve(extensions.size());
        const auto convert = [](const foundation::UTF8& s) { return std::filesystem::path(s); };
        std::transform(
            extensions.begin(), extensions.end(), std::inserter(path_set, path_set.end()), convert);
        return path_set;
    }
} // namespace enishi::core