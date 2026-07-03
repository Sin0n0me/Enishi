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

        std::vector<std::filesystem::path> find_assets(const std::filesystem::path& target_path,
            const std::unordered_set<std::filesystem::path>& target_extensions)
            const noexcept override;
        std::vector<std::filesystem::path> find_models(
            const std::filesystem::path& target_path) const noexcept override;
        std::vector<std::filesystem::path> find_shaders(
            const std::filesystem::path& target_path) const noexcept override;
        std::vector<std::filesystem::path> find_textures(
            const std::filesystem::path& target_path) const noexcept override;
        std::vector<std::filesystem::path> find_scripts(
            const std::filesystem::path& target_path) const noexcept override;

        foundation::Option<const types::ModelData&> get_model_data(
            const assets_system::AssetHandle& handle) const noexcept override;
        foundation::Option<const types::ShaderData&> get_shader_data(
            const assets_system::AssetHandle& handle) const noexcept override;
        foundation::Option<const types::TextureData&> get_texture_data(
            const assets_system::AssetHandle& handle) const noexcept override;

      public:
        void update(const types::DeltaTime& delta_time) override;

      private:
        template <typename T>
        foundation::Result<types::HandleId, SystemError> register_asset(T&& data) noexcept {
            const auto id = this->asset_registory.create();
            auto result = this->asset_registory.insert(id, std::move(data));
            if (result.is_err()) {
                this->asset_registory.destroy(id);
                return result.add_message("アセットデータの登録に失敗しました")
                    .propagation(SystemError::AssetSystemError);
            }
            return id;
        }

        foundation::Result<assets_system::AssetHandle, assets_system::AssetError> register_model(
            types::ModelData&& data) noexcept;

        foundation::Result<assets_system::AssetHandle, assets_system::AssetError>
        register_animation(types::ModelData&& data) noexcept;

        foundation::Result<assets_system::AssetHandle, assets_system::AssetError> register_shader(
            types::ShaderData&& data) noexcept;

        foundation::Result<assets_system::AssetHandle, assets_system::AssetError> register_texture(
            types::TextureData&& data) noexcept;

        foundation::Result<assets_system::AssetHandle, assets_system::AssetError> register_video(
            types::ModelData&& data) noexcept;

        foundation::Result<assets_system::AssetHandle, assets_system::AssetError> register_sound(
            types::ModelData&& data) noexcept;

        foundation::Result<assets_system::AssetHandle, assets_system::AssetError> register_script(
            types::ModelData&& data) noexcept;

        static std::unordered_set<std::filesystem::path> convert_hash_set(
            const std::vector<foundation::UTF8>& extensions) noexcept;
    };
} // namespace enishi::core