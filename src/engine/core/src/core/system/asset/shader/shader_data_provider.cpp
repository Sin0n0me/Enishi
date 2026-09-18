#include "shader_data_provider.h"
#include <foundation/path/path_utility.h>

namespace enishi::core {
    const std::filesystem::path SHADER_PATH = "./assets/shader";

    ShaderDataProvider::ShaderDataProvider(std::shared_ptr<platform::IAssetSystem> asset_system)
        : asset_system(std::move(asset_system)) {
    }

    foundation::Result<std::vector<platform::ShaderDataEntry>, platform::RenderError>
    ShaderDataProvider::get(
        std::span<const std::tuple<types::ShaderKind, std::filesystem::path>> paths) const {
        const auto path_object =
            this->asset_system->find_assets(SHADER_PATH, types::AssetKind::Shader);
        const auto pattern_shader_extensions =
            this->asset_system->get_extensions_pattern(types::AssetKind::Shader);
        const auto make_paths = [&](const std::filesystem::path& file_path) {
            const auto str_pattern = std::format(
                "{}{}", foundation::path_to_regex_str(file_path), pattern_shader_extensions);
            const std::regex pattern(str_pattern);
            return path_object.matched_file_name(pattern);
        };

        std::vector<platform::ShaderDataEntry> entries;
        for (const auto& [kind, path] : paths) {
            const auto shader_paths = make_paths(path);
            for (const auto& shader_path : shader_paths) {
                const auto asset_handle = this->asset_system->load_asset(shader_path)
                                              .add_message("シェーダーの読み込みに失敗しました");
                if (asset_handle.is_err()) {
                    return asset_handle.propagation(platform::RenderError::MakeError);
                }
                const auto shader_data =
                    this->asset_system->get_asset<types::AssetShaderData>(asset_handle.unwrap());
                if (shader_data.is_none()) {
                    return foundation::Error(
                        platform::RenderError::MakeError, "シェーダーデータが存在しません");
                }

                entries.emplace_back(platform::ShaderDataEntry{
                    .kind = kind,
                    .path = shader_path,
                    .data = shader_data.unwrap(),
                });
            }
        }

        return entries;
    }
} // namespace enishi::core
