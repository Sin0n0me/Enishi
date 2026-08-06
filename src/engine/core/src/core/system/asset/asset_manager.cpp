#include "asset_manager.h"
#include <assets_system/model/model_loader/model_loader.h>
#include <assets_system/shader/shader_loader.h>
#include <assets_system/texture/texture_loader.h>
#include <foundation/log/logger.h>

namespace enishi::core {
    template <typename T, typename... Args>
    AssetManager::AssetLoader insert(AssetManager::LoaderMap& map, Args&... args) {
        auto ptr = std::make_shared<T>(args...);

        for (const auto extension : ptr->get_supported_extension()) {
            map[extension].push_back(ptr);
        }

        return ptr;
    }

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
        this->asset_type_to_loader[assets_system::AssetType::Model] =
            insert<assets_system::ModelLoader>(this->extension_to_loader);
        this->asset_type_to_loader[assets_system::AssetType::Texture] =
            insert<assets_system::TextureLoader>(this->extension_to_loader);
        this->asset_type_to_loader[assets_system::AssetType::Shader] =
            insert<assets_system::ShaderLoader>(this->extension_to_loader);
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

        // 1つの拡張子が複数対応している場合判断がつかないので
        // 最初に正常に読み込めた値を返す
        for (const auto& loader : asset_iter->second) {
            auto result = loader->load(path);
            if (result.is_err()) {
                continue;
            }

            auto&& asset_data = result.unwrap_mut();
            if (const auto model_data = std::get_if<types::ModelData>(&asset_data)) {
                auto&& handle = this->register_model(std::move(*model_data));
                if (handle.is_ok()) {
                    this->path_to_handle[normalized_path] = handle.unwrap();
                    return handle;
                }
            }
            if (auto texture_data = std::get_if<types::TextureData>(&asset_data)) {
                auto&& handle = this->register_texture(std::move(*texture_data));
                if (handle.is_ok()) {
                    this->path_to_handle[normalized_path] = handle.unwrap();
                    return handle;
                }
            }
            if (auto shader_data = std::get_if<types::ShaderData>(&asset_data)) {
                auto&& handle = this->register_shader(std::move(*shader_data));
                if (handle.is_ok()) {
                    this->path_to_handle[normalized_path] = handle.unwrap();
                    return handle;
                }
            }
        }

        return foundation::Error(assets_system::AssetError::NotFound);
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

    assets_system::PathObjects AssetManager::find_models(
        const std::filesystem::path& target_path) const noexcept {
        const auto& extensions = this->get_extensions(assets_system::AssetType::Model);
        return this->find_assets(target_path, AssetManager::convert_hash_set(extensions));
    }

    assets_system::PathObjects AssetManager::find_shaders(
        const std::filesystem::path& target_path) const noexcept {
        const auto& extensions = this->get_extensions(assets_system::AssetType::Shader);
        return this->find_assets(target_path, AssetManager::convert_hash_set(extensions));
    }

    assets_system::PathObjects AssetManager::find_textures(
        const std::filesystem::path& target_path) const noexcept {
        const auto& extensions = this->get_extensions(assets_system::AssetType::Texture);
        return this->find_assets(target_path, AssetManager::convert_hash_set(extensions));
    }
    assets_system::PathObjects AssetManager::find_scripts(
        const std::filesystem::path& target_path) const noexcept {
        const auto& extensions = this->get_extensions(assets_system::AssetType::Script);
        return this->find_assets(target_path, AssetManager::convert_hash_set(extensions));
    }

    foundation::Option<const types::ModelData&> core::AssetManager::get_model_data(
        const assets_system::AssetHandle& handle) const noexcept {
        return this->asset_registory.get<types::ModelData>(handle.id);
    }

    foundation::Option<const types::ShaderData&> AssetManager::get_shader_data(
        const assets_system::AssetHandle& handle) const noexcept {
        return this->asset_registory.get<types::ShaderData>(handle.id);
    }

    foundation::Option<const types::TextureData&> AssetManager::get_texture_data(
        const assets_system::AssetHandle& handle) const noexcept {
        return this->asset_registory.get<types::TextureData>(handle.id);
    }

    foundation::UTF8 core::AssetManager::model_extensions_pattern(void) const noexcept {
        const auto& extensions = this->get_extensions(assets_system::AssetType::Model);
        return make_extension_regex(extensions);
    }

    foundation::UTF8 core::AssetManager::shader_extensions_pattern(void) const noexcept {
        const auto& extensions = this->get_extensions(assets_system::AssetType::Shader);
        return make_extension_regex(extensions);
    }

    foundation::UTF8 core::AssetManager::texture_extensions_pattern(void) const noexcept {
        const auto& extensions = this->get_extensions(assets_system::AssetType::Texture);
        return make_extension_regex(extensions);
    }

    foundation::UTF8 core::AssetManager::script_extensions_pattern(void) const noexcept {
        const auto& extensions = this->get_extensions(assets_system::AssetType::Script);
        return make_extension_regex(extensions);
    }

    bool enishi::core::AssetManager::should_close(void) {
        return false;
    }
    void enishi::core::AssetManager::pre_update(void) {
    }
    void core::AssetManager::update(const types::DeltaTime& delta_time) {
    }
    void enishi::core::AssetManager::post_update(void) {
    }
    void enishi::core::AssetManager::render(void) const {
    }

    foundation::Result<assets_system::AssetHandle, assets_system::AssetError>
    core::AssetManager::register_model(types::ModelData&& data) noexcept {
        const auto asset_id = this->register_asset(std::move(data));
        if (asset_id.is_err()) {
            return asset_id.propagation(assets_system::AssetError::AlreadyHasAsset)
                .add_message("モデルの追加に失敗しました");
        }
        return assets_system::AssetHandle{
            .id = asset_id.unwrap(),
            .type = assets_system::AssetType::Model,
        };
    }

    foundation::Result<assets_system::AssetHandle, assets_system::AssetError>
    core::AssetManager::register_shader(types::ShaderData&& data) noexcept {
        const auto asset_id = this->register_asset(std::move(data));
        if (asset_id.is_err()) {
            return asset_id.unwrap_err()
                .propagation(assets_system::AssetError::AlreadyHasAsset)
                .add_message("シェーダーの追加に失敗しました");
        }
        return assets_system::AssetHandle{
            .id = asset_id.unwrap(),
            .type = assets_system::AssetType::Shader,
        };
    }

    foundation::Result<assets_system::AssetHandle, assets_system::AssetError>
    core::AssetManager::register_texture(types::TextureData&& data) noexcept {
        const auto asset_id = this->register_asset(std::move(data));
        if (asset_id.is_err()) {
            return asset_id.propagation(assets_system::AssetError::AlreadyHasAsset)
                .add_message("テクスチャの追加に失敗しました");
        }
        return assets_system::AssetHandle{
            .id = asset_id.unwrap(),
            .type = assets_system::AssetType::Texture,
        };
    }

    std::vector<foundation::UTF8> core::AssetManager::get_extensions(
        const assets_system::AssetType asset_type) const {
        const auto iter = this->asset_type_to_loader.find(asset_type);
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