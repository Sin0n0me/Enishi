#include "helper.h"
#include <foundation/log/logger.h>
#include <foundation/path/path_utility.h>
#include <foundation/str/string_builder.h>

namespace enishi::render_pass {
    constexpr glm::vec4 CLEAR_COLOR = glm::vec4{0.25f, 0.25f, 0.25f, 0.25f};

    ShaderKindToData make_shader_map_presorted(
        const std::vector<platform::ShaderDataEntry>& entries) {
        ShaderKindToData map;

        auto projection = entries | std::views::transform([](const auto& entry) {
            return std::pair{entry.kind, entry.data};
        });

        for (auto group : projection | std::views::chunk_by([](const auto& a, const auto& b) {
                 return a.first == b.first;
             })) {
            types::ShaderKind kind = group.front().first;
            auto data_list = group |
                             std::views::transform([](const auto& pair) { return pair.second; }) |
                             std::ranges::to<std::vector>();
            map.emplace(kind, std::move(data_list));
        }

        return map;
    }

    foundation::Result<types::RenderHandle, ConstructError> make_render_target(
        types::ImageDescription&& description,
        const types::ImageFormat view_format,
        platform::IRenderer* const renderer) {
        const auto image_handle =
            renderer->create_image(description)
                .add_message("レンダーターゲット用イメージの作成に失敗しました");
        if (image_handle.is_err()) {
            return image_handle.propagation(ConstructError::Construct);
        }

        const auto image_view_description =
            types::ImageViewDescription::make_render_target_view_description(view_format);
        const auto result =
            renderer->create_render_target_view(image_handle.unwrap(), image_view_description)
                .add_message("レンダーターゲットの作成に失敗しました");

        if (result.is_err()) {
            return result.propagation(ConstructError::Construct);
        }
        auto& render_target_view = result.unwrap();
        if (!bool(render_target_view)) {
            return foundation::Error(ConstructError::Construct);
        }

        render_target_view->set_clear_color(CLEAR_COLOR);

        return render_target_view->get_handle();
    }

    foundation::Result<types::RenderHandle, ConstructError> make_depth_stencil(
        types::ImageDescription&& description,
        const types::ImageFormat view_format,
        platform::IRenderer* const renderer) {
        const auto image_handle = renderer->create_image(description)
                                      .add_message("深度ステンシル用イメージの作成に失敗しました");
        if (image_handle.is_err()) {
            return image_handle.propagation(ConstructError::Construct);
        }

        const auto image_view_description =
            types::ImageViewDescription::make_depth_stencil_view_description(view_format);
        const auto result =
            renderer->create_depth_stencil_view(image_handle.unwrap(), image_view_description)
                .add_message("深度ステンシルの作成に失敗しました");
        if (result.is_err()) {
            return result.propagation(ConstructError::Construct);
        }
        auto& render_target_view = result.unwrap();
        if (!bool(render_target_view)) {
            return foundation::Error(ConstructError::Construct);
        }

        return render_target_view->get_handle();
    }

    foundation::Result<types::RenderHandle, ConstructError> make_rasterizer(
        types::RasterizerStateDescription&& description, platform::IRenderer* const renderer) {
        const auto state = renderer->create_rasterizer(description)
                               .add_message("ラスタライザの作成に失敗しました");
        if (state.is_err()) {
            return state.propagation(ConstructError::Construct);
        }

        return state.unwrap();
    }

    foundation::Result<types::RenderHandle, ConstructError> make_depth_stencil_state(
        types::DepthStencilStateDescription&& description, platform::IRenderer* const renderer) {
        const auto state = renderer->create_depth_stencil(description)
                               .add_message("デプスステンシルの作成に失敗しました");
        if (state.is_err()) {
            return state.propagation(ConstructError::Construct);
        }

        return state.unwrap();
    }

    foundation::Result<types::RenderHandle, ConstructError> make_blend_state(
        types::BlendStateDescription&& description, platform::IRenderer* const renderer) {
        const auto state =
            renderer->create_blend(description).add_message("ブレンドの作成に失敗しました");
        if (state.is_err()) {
            return state.propagation(ConstructError::Construct);
        }

        return state.unwrap();
    }

    foundation::Result<types::RenderHandle, ConstructError> make_sampler_state(
        types::SamplerStateDescription&& description, platform::IRenderer* const renderer) {
        const auto state =
            renderer->create_sampler(description).add_message("サンプラーの作成に失敗しました");
        if (state.is_err()) {
            return state.propagation(ConstructError::Construct);
        }

        return state.unwrap();
    }

    foundation::Result<std::vector<ShaderResult>, ConstructError> make_shaders(
        platform::IRenderer* const renderer, ShaderKindToData&& shaders_map) {
        auto shader_result = std::vector<ShaderResult>();

        for (const auto& [kind, shaders] : shaders_map) {
            for (const auto& shader : shaders) {
                auto result = make_shader(kind, *shader, renderer)
                                  .add_message("シェーダーの作成に失敗しました");
                if (result.is_err()) {
                    return std::move(result).unwrap_err();
                }
                const auto& handles = result.unwrap();
                shader_result.emplace_back(std::move(result).unwrap());
            }
        }

        return shader_result;
    }

    foundation::Result<ShaderResult, ConstructError> make_shader(const types::ShaderKind kind,
        const types::ShaderData& shader_data,
        platform::IRenderer* const renderer) {
        // シェーダーの作成
        const auto shader = renderer->create_shader(kind, shader_data);
        if (shader.is_err()) {
            return foundation::Error(ConstructError::Construct);
        }

        // シェーダーリフレクションの作成(こちらは最悪失敗してもよい)
        const auto& shader_reflection = renderer->create_shader_reflection(shader_data);

        // 頂点のシェーダーの場合はシェーダーデータからリフレクション作成
        auto input_layout = types::RenderHandle{};
        if (kind == types::ShaderKind::Vertex) {
            const auto result_input_layout =
                renderer->create_vertex_layout_from_shader_data(shader_data)
                    .add_message("頂点レイアウトの作成に失敗しました");
            if (result_input_layout.is_err()) {
                return result_input_layout.propagation(ConstructError::Construct);
            }
            input_layout = result_input_layout.unwrap();
        }

        return ShaderResult{
            .shader = shader.unwrap(),
            .shader_reflection = shader_reflection.unwrap_or_default(),
            .input_layout = input_layout,
        };
    }
} // namespace enishi::render_pass