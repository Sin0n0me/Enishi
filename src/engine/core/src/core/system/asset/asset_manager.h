#pragma once
#include "../../errors/errors.h"
#include "../interface_system.h"
#include <assets_system/asset_handle.h>
#include <assets_system/interface_asset_system.h>
#include <ecs/registory.h>
#include <filesystem>
#include <foundation/str/str.h>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace enishi::core {
    class AssetManager : public assets_system::IAssetSystem, public ISystem {
      private:
        using LoaderMap = std::unordered_map<foundation::UTF8,
            std::vector<std::shared_ptr<assets_system::IAssetLoader>>>;

      private:
        ecs::Registory asset_registory;
        std::unordered_map<std::filesystem::path, assets_system::AssetHandle> path_to_handle;
        LoaderMap extension_to_loader;

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

        foundation::Option<const types::ModelData&> get_model_data(
            const assets_system::AssetHandle& handle) const noexcept override;

      public:
        void update(const types::DeltaTime& delta_time) override;

      private:
        foundation::Result<types::HandleId, SystemError> register_model(
            const types::ModelData& data) noexcept;

        foundation::Result<types::HandleId, SystemError> load_animation(
            const std::filesystem::path& path) noexcept;

        foundation::Result<types::HandleId, SystemError> load_shader(
            const std::filesystem::path& path) noexcept;

        foundation::Result<types::HandleId, SystemError> load_texture(
            const std::filesystem::path& path) noexcept;

        foundation::Result<types::HandleId, SystemError> load_video(
            const std::filesystem::path& path) noexcept;

        foundation::Result<types::HandleId, SystemError> load_sound(
            const std::filesystem::path& path) noexcept;

        foundation::Result<types::HandleId, SystemError> load_script(
            const std::filesystem::path& path) noexcept;
    };
} // namespace enishi::core