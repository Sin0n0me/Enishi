#include "background_render_pass_constructor.h"
#include "../helper.h"
#include <foundation/log/logger.h>
#include <foundation/path/path_utility.h>
#include <foundation/str/string_builder.h>
#include <render_pass/constructor/shadow/shadow_map_render_pass_constructor.h>
#include <render_pass/render_pass.h>

namespace enishi::render_pass {
    constexpr char VS_FILE_NAME[] = "vs_clear_wall";
    constexpr char PS_FILE_NAME[] = "ps_clear_wall";

    foundation::Result<std::shared_ptr<platform::IRenderPass>, ConstructError>
    enishi::render_pass::BackgroundRenderPassConstructor::make(platform::IRenderer* const renderer,
        const platform::IWindow* window,
        const platform::IShaderDataProvider* shader_data_provider,
        std::span<platform::IRenderPass* const> dependency_render_passes) {
        types::PipelineDescription description{
            .topology = types::PrimitiveTopology::TriangleList,
        };

        const auto shader_paths = this->get_paths();
        const auto result_shader_entries = shader_data_provider->get(shader_paths);
        if (result_shader_entries.is_err()) {
            return result_shader_entries.propagation(ConstructError::Construct);
        }
        auto&& shader_entries = make_shader_map_presorted(result_shader_entries.unwrap());
        const auto result_shader = make_shaders(renderer, std::move(shader_entries));
        if (result_shader.is_err()) {
            return foundation::Error(ConstructError::Construct);
        }
        auto&& shaders = result_shader.unwrap();
        for (auto& shader : shaders) {
            if (shader.shader.is_valid()) {
                description.shaders.emplace_back(shader.shader);
            }
            if (shader.shader_reflection.is_valid()) {
                description.shader_reflections.emplace_back(shader.shader_reflection);
            }
        }

        const auto opt_window_size = window->get_size();
        if (opt_window_size.is_none()) {
            return foundation::Error(ConstructError::Construct);
        }
        auto window_size = opt_window_size.unwrap().to_glm_ivec2();

        // レンダーターゲットの作成
        auto rtv =
            make_render_target(types::ImageDescription::make_default_render_target(window_size),
                types::ImageFormat::BGRA8_UNORM,
                renderer);
        if (rtv.is_err()) {
            return std::move(rtv).unwrap_err();
        }
        description.render_target_view = rtv.unwrap();

        // 深度ステンシルの作成
        auto dsv = make_depth_stencil(types::ImageDescription::make_depth_stencil(
                                          window_size, types::ImageFormat::D24_UNORM_S8_UINT),
            types::ImageFormat::BGRA8_UNORM,
            renderer);
        if (dsv.is_err()) {
            return std::move(dsv).unwrap_err();
        }
        description.depth_stencil_view = dsv.unwrap();

        // ブレンドステートの作成
        auto blend_state = make_blend_state(
            types::BlendStateDescription{
                .alpha_to_coverage = true,
                .independent_blend = true,
                .render_targets = types::RenderTargetBlendState::default_blend_state(),
            },
            renderer);
        if (blend_state.is_err()) {
            return std::move(blend_state).unwrap_err();
        }
        description.blend_state = blend_state.unwrap();

        // ラスタライザの作成
        auto rasterizer = make_rasterizer(
            types::RasterizerStateDescription{
                .cull_mode = types::CullMode::None,
                .front_face = types::FrontFace::CounterClockwise,
            },
            renderer);
        if (rasterizer.is_err()) {
            return std::move(rasterizer).unwrap_err();
        }
        description.rasterizer_state = rasterizer.unwrap();

        // レンダーパスの生成
        auto render_pass = std::make_shared<RenderPass>();
        const auto render_pass_result = render_pass->make_from_description(
            description,
            dependency_render_passes,
            this->get_render_pass_name(),
            this->get_node(),
            this->get_dependencies());
        if (render_pass_result.is_err()) {
            return render_pass_result.propagation(ConstructError::Construct);
        }

        return render_pass;
    }

    foundation::DependencyNode BackgroundRenderPassConstructor::get_node(void) const noexcept {
        return NODE;
    }

    foundation::DependencyBounds BackgroundRenderPassConstructor::get_dependencies(
        void) const noexcept {
        return foundation::DependencyBounds{.precedents = {
                                                ShadowMapRenderPassConstructor::NODE,
                                            }};
    }

    std::vector<std::tuple<types::ShaderKind, std::filesystem::path>>
    enishi::render_pass::BackgroundRenderPassConstructor::get_paths(void) const noexcept {
        return {
            {types::ShaderKind::Vertex, VS_FILE_NAME},
            {types::ShaderKind::Pixel, PS_FILE_NAME},
        };
    }

    foundation::UTF8 enishi::render_pass::BackgroundRenderPassConstructor::get_render_pass_name(
        void) const noexcept {
        return foundation::UTF8();
    }
} // namespace enishi::render_pass