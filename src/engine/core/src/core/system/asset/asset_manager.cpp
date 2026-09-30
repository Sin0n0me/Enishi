#include "asset_manager.h"
#include <assets_system/audio/audio_loader.h>
#include <assets_system/model/model_loader/model_loader.h>
#include <assets_system/shader/shader_loader.h>
#include <assets_system/texture/texture_loader.h>
#include <foundation/log/logger.h>
#include <variant>

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
        this->add_loader<assets_system::AudioLoader>(types::AssetKind::Audio);
    }

    foundation::Result<types::AssetHandle, platform::AssetError> AssetManager::load_asset(
        const std::filesystem::path& path) noexcept {
        // すでに読み込み済み、または読み込み中の場合はそのまま保管しているハンドルを返す
        // (path_to_handleへの登録は読み込み完了を待たず即座に行うため、これだけで多重読み込みを防げる)
        const auto normalized_path = path.lexically_normal();
        const auto iter = this->path_to_handle.find(normalized_path);
        if (iter != this->path_to_handle.end()) {
            return iter->second;
        }

        if (!path.has_extension()) {
            return foundation::Error(platform::AssetError::NotFound);
        }

        // 拡張子に応じたアセットローダー候補を探す
        // ここまではIOを伴わないため、この関数の呼び出し元スレッドで処理して問題ない
        const auto extention = path.extension();
        const auto asset_iter = this->extension_to_loader.find(extention.string<char>());
        if (asset_iter == this->extension_to_loader.end()) {
            return foundation::Error(platform::AssetError::NotFound,
                std::format("not found loader. target: {}", path.string<char>()));
        }

        const auto& candidates = asset_iter->second;
        if (candidates.empty()) {
            return foundation::Error(platform::AssetError::NotFound);
        }

        // ハンドルはIOの完了を待たずにこの場で発行する
        // (typeは最有力候補である先頭ローダーの対応アセット種別を暫定的に採用する。
        //  1拡張子に複数ローダーが対応するケースは稀であり、通常はここで確定する)
        const auto handle = types::AssetHandle{
            .id = this->asset_registry.create(),
            .type = candidates.front()->get_target_asset_type(),
        };

        this->path_to_handle[normalized_path] = handle;
        this->set_asset_state(handle, types::AssetState::Queued);
        this->request_load(normalized_path, handle, candidates);

        return handle;
    }

    void AssetManager::release_asset(const types::AssetHandle& handle) noexcept {
    }

    foundation::Option<const std::filesystem::path&> AssetManager::get_asset_file_name(
        const types::AssetHandle& handle) const noexcept {
        return foundation::Option<const std::filesystem::path&>();
    }

    foundation::PathObjects AssetManager::find_assets(const std::filesystem::path& target_path,
        const std::unordered_set<std::filesystem::path>& target_extensions) const noexcept {
        foundation::PathObjects matched_files;
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

    foundation::PathObjects AssetManager::find_assets(const std::filesystem::path& target_path,
        const types::AssetKind asset_kind) const noexcept {
        const auto& extensions = this->get_extensions(asset_kind);
        return this->find_assets(target_path, AssetManager::convert_hash_set(extensions));
    }

    void AssetManager::request_load(const std::filesystem::path& path,
        const types::AssetHandle& handle,
        const std::vector<AssetLoader>& candidates) const {
        // キャプチャは安全のためにコピーで行う
        auto job = [this, path, handle, candidates] {
            this->set_asset_state(handle, types::AssetState::Loading);

            // 1つの拡張子が複数ローダーに対応している場合, 最初に正常に読み込めた結果を採用する
            for (const auto& loader : candidates) {
                auto result = loader->load(path);
                if (result.is_ok()) {
                    foundation::Logger::info(std::format("loaded path: {}", path.string<char>()));
                    this->set_asset_state(handle, types::AssetState::Loaded);
                    return result;
                }
                foundation::Logger::warning(result.unwrap_err().get_message());
            }

            return foundation::Result<types::AssetData, assets_system::AssetError>(
                foundation::Error(assets_system::AssetError::NotFound,
                    std::format("読み込みに失敗しました. target: {}", path.string<char>())));
        };

        // 実際のIOスレッド管理・完了結果の受け渡しはスケジューラの責務
        this->load_scheduler.submit(handle, path, std::move(job));
    }

    void AssetManager::set_asset_state(
        const types::AssetHandle& handle, const types::AssetState state) const noexcept {
        const std::lock_guard<std::mutex> lock(this->state_mutex);
        this->asset_states[handle] = state;
    }

    void AssetManager::commit_completed_load(CompletedLoad&& completed) const noexcept {
        if (completed.result.is_err()) {
            foundation::Logger::warning(
                std::format("アセットの読み込みに失敗しました. path: {}, message: {}",
                    completed.path.string<char>(),
                    completed.result.unwrap_err().get_message()));

            this->path_to_handle.erase(completed.path);
            this->asset_registry.destroy(completed.handle.id);
            this->set_asset_state(completed.handle, types::AssetState::Failed);
            return;
        }

        // Registryへの実データ挿入は必ずメインスレッドから行う(ComponentPoolはスレッドセーフでないため)
        auto asset_data = std::move(completed.result).unwrap_mut();
        const auto insert_result = this->insert_asset(completed.handle.id, std::move(asset_data));

        if (insert_result.is_err()) {
            foundation::Logger::error(insert_result.unwrap_err().get_message());

            this->path_to_handle.erase(completed.path);
            this->asset_registry.destroy(completed.handle.id);
            this->set_asset_state(completed.handle, types::AssetState::Failed);
            return;
        }

        this->set_asset_state(completed.handle, types::AssetState::Loaded);
    }

    void AssetManager::drain_completed_loads(void) const {
        for (;;) {
            auto completed = this->load_scheduler.try_take_completed();
            if (completed.is_none()) {
                break;
            }

            this->commit_completed_load(std::move(completed).unwrap_mut());
        }
    }

    void AssetManager::ensure_asset_loaded(const types::AssetHandle& handle) const noexcept {
        // Queued/Loading以外(NotLoaded/Loaded/Failed)であれば何もしない
        // 待っても新しい結果は来ない(そもそも投入されていない, 既に確定済み)ため
        if (types::is_inactive_state(this->get_asset_state(handle))) {
            return;
        }

        // スケジューラ側にこのハンドルの完了が来るまでブロック
        // 届いたら即座にRegistryへ反映する
        auto completed = this->load_scheduler.wait_and_take_completed(handle);
        if (completed.is_none()) {
            // 通常は起こらないが、万一スケジューラ側に見当たらなければ状態をFailedにして諦める
            foundation::Logger::error("読み込み待機中に結果を取得できませんでした");
            this->set_asset_state(handle, types::AssetState::Failed);
            return;
        }

        this->commit_completed_load(std::move(completed).unwrap_mut());
    }

    types::AssetState AssetManager::get_asset_state(
        const types::AssetHandle& handle) const noexcept {
        const std::lock_guard<std::mutex> lock(this->state_mutex);

        const auto iter = this->asset_states.find(handle);
        if (iter == this->asset_states.end()) {
            return types::AssetState::NotLoaded;
        }
        return iter->second;
    }

    foundation::UTF8 AssetManager::get_extensions_pattern(
        const types::AssetKind asset_kind) const noexcept {
        const auto& extensions = this->get_extensions(asset_kind);
        return make_extension_regex(extensions);
    }

    foundation::Option<const types::AssetData&> AssetManager::get_asset_data(
        const types::AssetHandle& handle) const noexcept {
        auto&& cached = this->asset_registry.get_const<types::AssetData>(handle.id);
        if (cached.is_some()) {
            return cached;
        }

        // 読み込み中であれば、ここで完了を待ってからもう一度取得する
        this->ensure_asset_loaded(handle);
        return this->asset_registry.get_const<types::AssetData>(handle.id);
    }

    std::vector<foundation::UTF8> AssetManager::get_extensions(
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