#pragma once
#include "../../errors/errors.h"
#include "../interface_system.h"
#include <assets_system/asset_handle.h>
#include <assets_system/interface_asset_loader.h>
#include <assets_system/interface_asset_system.h>
#include <ecs/registory.h>
#include <engine_types/assets/model/model_data.h>
#include <engine_types/assets/shader/shader_data.h>
#include <engine_types/assets/texture/texture_data.h>
#include <filesystem>
#include <foundation/str/str.h>
#include <memory>
#include <thread>
#include <unordered_map>
#include <vector>

namespace enishi::core {
    class AssetManager : public assets_system::IAssetSystem, public ISystem {
      private:
        using AssetLoader = std::shared_ptr<assets_system::IAssetLoader>;
        using LoaderMap = std::unordered_map<foundation::UTF8, std::vector<AssetLoader>>;

      private:
        ecs::Registory asset_registory;
        std::unordered_map<std::filesystem::path, assets_system::AssetHandle> path_to_handle;
        LoaderMap extension_to_loader;
        std::unordered_map<assets_system::AssetType, AssetLoader> asset_type_to_loader;

      public:
        explicit AssetManager(void);

        foundation::Result<assets_system::AssetHandle, assets_system::AssetError> load_asset(
            const std::filesystem::path& path) noexcept override;

        void release_asset(const assets_system::AssetHandle& handle) noexcept override;

        foundation::Option<const std::filesystem::path&> get_asset_file_name(
            const assets_system::AssetHandle& handle) const noexcept override;

        assets_system::PathObjects find_assets(const std::filesystem::path& target_path,
            const std::unordered_set<std::filesystem::path>& target_extensions)
            const noexcept override;
        assets_system::PathObjects find_models(
            const std::filesystem::path& target_path) const noexcept override;
        assets_system::PathObjects find_shaders(
            const std::filesystem::path& target_path) const noexcept override;
        assets_system::PathObjects find_textures(
            const std::filesystem::path& target_path) const noexcept override;
        assets_system::PathObjects find_scripts(
            const std::filesystem::path& target_path) const noexcept override;

        foundation::Option<const assets_system::AssetModelData&> get_model_data(
            const assets_system::AssetHandle& handle) const noexcept override;
        foundation::Option<const assets_system::AssetShaderData&> get_shader_data(
            const assets_system::AssetHandle& handle) const noexcept override;
        foundation::Option<const assets_system::AssetTextureData&> get_texture_data(
            const assets_system::AssetHandle& handle) const noexcept override;

        foundation::UTF8 model_extensions_pattern(void) const noexcept override;
        foundation::UTF8 shader_extensions_pattern(void) const noexcept override;
        foundation::UTF8 texture_extensions_pattern(void) const noexcept override;
        foundation::UTF8 script_extensions_pattern(void) const noexcept override;

      public:
        bool should_close(void) override;
        void pre_update(void) override;
        void update(const types::DeltaTime& delta_time) override;
        void post_update(void) override;
        void render(void) const override;

      private:
        template <typename T, typename... Args>
        std::shared_ptr<T> add_loader(const assets_system::AssetType asset_type, Args&&... args) {
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
            const assets_system::AssetType asset_type) const;

        static std::unordered_set<std::filesystem::path> convert_hash_set(
            const std::vector<foundation::UTF8>& extensions) noexcept;
    };
} // namespace enishi::core