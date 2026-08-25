#include "helper.h"
#include <foundation/log/logger.h>
#include <foundation/path/path_utility.h>
#include <foundation/str/string_builder.h>
#include <settings.h>

namespace enishi {
    const std::filesystem::path SHADER_PATH = "./assets/shader";

    foundation::Result<types::RenderHandle, core::SystemError> make_render_target(
        types::ImageDescription&& description,
        types::ImageFormat&& view_format,
        platform::IRenderer* const renderer) {
        const auto image_handle =
            renderer->create_image(description)
                .add_message("レンダーターゲット用イメージの作成に失敗しました");
        if (image_handle.is_err()) {
            return image_handle.propagation(core::SystemError::ConstructRenderPassError);
        }

        const auto image_view_description =
            types::ImageViewDescription::make_render_target_view_description(view_format);
        const auto result =
            renderer->create_render_target_view(image_handle.unwrap(), image_view_description)
                .add_message("レンダーターゲットの作成に失敗しました");

        if (result.is_err()) {
            return result.propagation(core::SystemError::ConstructRenderPassError);
        }
        auto& render_target_view = result.unwrap();
        if (!bool(render_target_view)) {
            return foundation::Error(core::SystemError::ConstructRenderPassError);
        }

        render_target_view->set_clear_color(CLEAR_COLOR);

        return render_target_view->get_handle();
    }

    foundation::Result<types::RenderHandle, core::SystemError> make_depth_stencil(
        types::ImageDescription&& description,
        types::ImageFormat&& view_format,
        platform::IRenderer* const renderer) {
        const auto image_handle = renderer->create_image(description)
                                      .add_message("深度ステンシル用イメージの作成に失敗しました");
        if (image_handle.is_err()) {
            return image_handle.propagation(core::SystemError::ConstructRenderPassError);
        }

        const auto image_view_description =
            types::ImageViewDescription::make_depth_stencil_view_description(view_format);
        const auto result =
            renderer->create_depth_stencil_view(image_handle.unwrap(), image_view_description)
                .add_message("深度ステンシルの作成に失敗しました");
        if (result.is_err()) {
            return result.propagation(core::SystemError::ConstructRenderPassError);
        }
        auto& render_target_view = result.unwrap();
        if (!bool(render_target_view)) {
            return foundation::Error(core::SystemError::ConstructRenderPassError);
        }

        return render_target_view->get_handle();
    }

    foundation::Result<types::RenderHandle, core::SystemError> make_rasterizer(
        types::RasterizerStateDescription&& description, platform::IRenderer* const renderer) {
        const auto state = renderer->create_rasterizer(description)
                               .add_message("ラスタライザの作成に失敗しました");
        if (state.is_err()) {
            return state.propagation(core::SystemError::ConstructRenderPassError);
        }

        return state.unwrap();
    }

    foundation::Result<types::RenderHandle, core::SystemError> make_depth_stencil_state(
        types::DepthStencilStateDescription&& description, platform::IRenderer* const renderer) {
        const auto state = renderer->create_depth_stencil(description)
                               .add_message("デプスステンシルの作成に失敗しました");
        if (state.is_err()) {
            return state.propagation(core::SystemError::ConstructRenderPassError);
        }

        return state.unwrap();
    }

    foundation::Result<types::RenderHandle, core::SystemError> make_blend_state(
        types::BlendStateDescription&& description, platform::IRenderer* const renderer) {
        const auto state =
            renderer->create_blend(description).add_message("ブレンドの作成に失敗しました");
        if (state.is_err()) {
            return state.propagation(core::SystemError::ConstructRenderPassError);
        }

        return state.unwrap();
    }

    foundation::Result<types::RenderHandle, core::SystemError> make_sampler_state(
        types::SamplerStateDescription&& description, platform::IRenderer* const renderer) {
        const auto state =
            renderer->create_sampler(description).add_message("サンプラーの作成に失敗しました");
        if (state.is_err()) {
            return state.propagation(core::SystemError::ConstructRenderPassError);
        }

        return state.unwrap();
    }

    foundation::Result<ShaderResult, core::SystemError> make_shader_from_file_paths(
        const types::ShaderKind kind,
        const std::vector<std::filesystem::path>& paths,
        platform::IRenderer* const renderer,
        assets_system::IAssetSystem* const asset_system) {
        const bool multiple_shaders_found = 1 < paths.size();
        if (multiple_shaders_found) {
            foundation::Logger::warning("複数のシェーダーファイルが見つかりました");
        }

        // シェーダーの作成
        // 複数のシェーダーが見つかった場合は最初に正常に作成できたシェーダーを使用
        foundation::StringBuilder error_messages;
        for (const auto& path : paths) {
            auto result = make_shader(kind, path, renderer, asset_system);
            if (result.is_err()) {
                error_messages.push_back(result.unwrap_err().get_message());
                continue;
            }

            return result;
        }

        return foundation::Error(
            core::SystemError::ConstructRenderPassError, error_messages.join("\n"));
    }

    foundation::Result<std::vector<ShaderResult>, core::SystemError> make_shaders(
        platform::IRenderer* const renderer,
        assets_system::IAssetSystem* const asset_system,
        std::unordered_map<types::ShaderKind, std::vector<std::filesystem::path>>&& paths) {
        const auto shader_paths = asset_system->find_shaders(SHADER_PATH);
        const auto pattern_shader_extensions = asset_system->shader_extensions_pattern();
        const auto make_paths = [&](const std::filesystem::path& file_path) {
            const auto str_pattern = std::format(
                "{}{}", foundation::path_to_regex_str(file_path), pattern_shader_extensions);
            const std::regex pattern(str_pattern);
            return shader_paths.find(pattern);
        };

        auto shader_result = std::vector<ShaderResult>();

        // TODO
        // 複数の頂点シェーダーが来た場合バグる
        for (auto& vertex_file_name : paths[types::ShaderKind::Vertex]) {
            const auto vertex_file = SHADER_PATH / vertex_file_name;
            const auto paths = make_paths(vertex_file);
            auto result = make_shader_from_file_paths(
                types::ShaderKind::Vertex, paths, renderer, asset_system)
                              .add_message("頂点シェーダーの作成に失敗しました");
            if (result.is_err()) {
                return std::move(result).unwrap_err();
            }
            const auto& handles = result.unwrap();
            shader_result.emplace_back(std::move(result).unwrap());
        }

        for (auto& fragment_file_name : paths[types::ShaderKind::Pixel]) {
            const auto pixel_file = SHADER_PATH / fragment_file_name;
            const auto paths = make_paths(pixel_file);
            auto result = make_shader_from_file_paths(
                types::ShaderKind::Fragment, paths, renderer, asset_system)
                              .add_message("フラグメントシェーダーの作成に失敗しました");
            if (result.is_err()) {
                return result.propagation(core::SystemError::ConstructRenderPassError);
            }
            shader_result.emplace_back(std::move(result).unwrap());
        }

        return shader_result;
    }

    foundation::Result<ShaderResult, core::SystemError> make_shader(const types::ShaderKind kind,
        const std::filesystem::path& path,
        platform::IRenderer* const renderer,
        assets_system::IAssetSystem* const asset_system) {
        // ファイル読み込み
        const auto asset_handle = asset_system->load_asset(path);
        if (asset_handle.is_err()) {
            return asset_handle.propagation(core::SystemError::ConstructRenderPassError);
        }
        const auto shader_data = asset_system->get_shader_data(asset_handle.unwrap());
        if (shader_data.is_none()) {
            return asset_handle.propagation(core::SystemError::ConstructRenderPassError);
        }

        // シェーダーの作成
        const auto& raw_shader_data = *shader_data.unwrap();
        const auto shader = renderer->create_shader(kind, raw_shader_data);
        if (shader.is_err()) {
            return asset_handle.propagation(core::SystemError::ConstructRenderPassError);
        }

        // シェーダーリフレクションの作成(こちらは最悪失敗してもよい)
        const auto& shader_reflection = renderer->create_shader_reflection(raw_shader_data);

        // 頂点のシェーダーの場合はシェーダーデータからリフレクション作成
        auto input_layout = types::RenderHandle{};
        if (kind == types::ShaderKind::Vertex) {
            const auto result_input_layout =
                renderer->create_vertex_layout_from_shader_data(raw_shader_data)
                    .add_message("頂点レイアウトの作成に失敗しました");
            if (result_input_layout.is_err()) {
                return result_input_layout.propagation(core::SystemError::ConstructRenderPassError);
            }
            input_layout = result_input_layout.unwrap();
        }

        return ShaderResult{
            .shader = shader.unwrap(),
            .shader_reflection = shader_reflection.unwrap_or_default(),
            .input_layout = input_layout,
        };
    }
} // namespace enishi