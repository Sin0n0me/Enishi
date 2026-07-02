#include "asset_manager.h"
#include <assets_system/model/model_loader/model_loader.h>
#include <assets_system/shader/shader_loader.h>
#include <assets_system/texture/texture_loader.h>
#include <foundation/log/logger.h>

namespace enishi::core {
    template <typename T, typename... Args>
    void insert(AssetManager::LoaderMap& map, Args&... args) {
        auto ptr = std::make_shared<T>(args...);

        for (const auto extension : ptr->get_supported_extension()) {
            map[extension].push_back(extension, ptr);
        }
    }

    AssetManager::AssetManager(void) {
        insert<assets_system::ModelLoader>(this->extension_to_loader);
        insert<assets_system::TextureLoader>(this->extension_to_loader);
        insert<assets_system::ShaderLoader>(this->extension_to_loader);
    }

    foundation::Result<assets_system::AssetHandle, assets_system::AssetError>
    AssetManager::load_asset(const std::filesystem::path& path) noexcept {
        // すでにAssetを保持している場合はそのまま保管しているハンドルを返す
        const auto iter = this->path_to_handle.find(path);
        if (iter != this->path_to_handle.end()) {
            return iter->second;
        }

        if (!path.has_extension()) {
            return foundation::Error(assets_system::AssetError::NotFound);
        }

        const auto extention = path.extension();
        const auto asset_iter = this->extension_to_loader.find(extention.string<char>());
        if (asset_iter == this->extension_to_loader.end()) {
            return foundation::Error(assets_system::AssetError::NotFound);
        }

        for (const auto& loader : asset_iter->second) {
            const auto result = loader->load(path);
            if (result.is_err()) {
                continue;
            }

            auto& asset_data = result.value();
            if (const auto model_data = std::get_if<types::ModelData>(&asset_data)) {
                const auto asset_id = this->register_model(std::move(*model_data));
                if (asset_id.is_err()) {
                    return asset_id.propagation(assets_system::AssetError::InvalidAssetData);
                }
                return assets_system::AssetHandle{
                    .id = asset_id.value(),
                    .type = assets_system::AssetType::Model,
                };
            }
            if (auto texture_data = std::get_if<types::TextureData>(&asset_data)) {
            }
            if (auto shader_data = std::get_if<types::ShaderData>(&asset_data)) {
            }
        }
    }

    void AssetManager::release_asset(const assets_system::AssetHandle& handle) noexcept {
    }

    foundation::Option<const std::filesystem::path&> AssetManager::get_asset_file_name(
        const assets_system::AssetHandle& handle) const noexcept {
        return foundation::Option<const std::filesystem::path&>();
    }

    std::vector<std::filesystem::path> AssetManager::find_assets(
        const std::filesystem::path& target_path,
        const std::unordered_set<std::filesystem::path>& target_extensions) const noexcept {
        std::vector<std::filesystem::path> matched_files;
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
                        matched_files.push_back(entry.path().string());
                    }
                }
            }
        }

        return matched_files;
    }

    foundation::Option<const types::ModelData&> core::AssetManager::get_model_data(
        const assets_system::AssetHandle& handle) const noexcept {
        return this->asset_registory.get<types::ModelData>(handle.id);
    }

    void core::AssetManager::update(const types::DeltaTime& delta_time) {
    }

    foundation::Result<types::HandleId, SystemError> AssetManager::load_model(
        const std::filesystem::path& path) noexcept {
        auto model = this->model_loader.load(path);
        if (model.is_err()) {
            return model.add_message("モデルデータの読み込みに失敗しました")
                .propagation(SystemError::AssetSystemError);
        }

        auto result = this->asset_registory.insert(id, model.value());
        if (result.is_err()) {
            return model.add_message("モデルデータの登録に失敗しました")
                .propagation(SystemError::AssetSystemError);
        }

        return id;
    }

    foundation::Result<types::HandleId, SystemError> core::AssetManager::register_model(
        const types::ModelData& data) noexcept {
        const auto id = this->asset_registory.create();
        auto result = this->asset_registory.insert(id, data);
        if (result.is_err()) {
            return foundation::Error(
                SystemError::AssetSystemError, "モデルデータの登録に失敗しました");
        }

        return foundation::Result<types::HandleId, SystemError>();
    }
} // namespace enishi::core