#include "model_render_pass_constructor.h"
#include <debug/log/logger.h>
#include <foundation/path/path_utility.h>
#include <foundation/str/string_builder.h>
#include <render_pass/constructor/background/background_render_pass_constructor.h>
#include <render_pass/constructor/pipeline_description_builder.h>
#include <render_pass/constructor/shadow/shadow_map_render_pass_constructor.h>
#include <render_pass/render_pass.h>
#include <utility>

// #include <platform/window/interface_window.h>

namespace enishi::render_pass {
    ModelRenderPassConstructor::ModelRenderPassConstructor(
        platform::RenderBackendDescription backend)
        : backend(std::move(backend)) {
    }

    foundation::Result<std::shared_ptr<platform::IRenderPass>, ConstructError>
    enishi::render_pass::ModelRenderPassConstructor::make(platform::IRenderer* const renderer,
        const platform::IWindow* window,
        const platform::IShaderDataProvider* shader_data_provider,
        std::span<platform::IRenderPass* const> dependency_render_passes) {
        types::RenderHandle render_target;
        types::RenderHandle shadow_depth;
        for (auto& pass : dependency_render_passes) {
            if (pass->get_name() == ShadowMapRenderPassConstructor::RENDER_PASS_NAME) {
                shadow_depth = pass->get_sampled_depth();
            }
            if (pass->get_name() == BackgroundRenderPassConstructor::RENDER_PASS_NAME) {
                render_target = pass->get_render_target();
            }
        }

        return PipelineDescriptionConstructer{renderer, window}
            .add_topology(types::PrimitiveTopology::TriangleList)
            ->add_shaders(shader_data_provider, this->get_paths())
            ->add_shader_resource(shadow_depth)
            ->add_render_target_view(render_target)
            ->add_depth_stencil_view(this->backend.depth_format)
            ->add_rasterizer_state(types::RasterizerStateDescription{
                .cull_mode = types::CullMode::None,
                .front_face = types::FrontFace::CounterClockwise,
            })
            ->add_depth_stencil_state(types::DepthStencilStateDescription{
                .depth =
                    types::DepthState{
                        .enabled = true,
                        .write_enabled = true,
                        .compare_operator = types::CompareOperator::LessEqual,
                    },
                .stencil =
                    types::StencilState{
                        .enabled = false,
                        .front = types::StencilFaceState{},
                        .back = types::StencilFaceState{},
                    },
            })
            ->build_render_pass(this);
    }

    foundation::DependencyNode ModelRenderPassConstructor::get_node(void) const noexcept {
        return NODE;
    }

    foundation::DependencyBounds ModelRenderPassConstructor::get_dependencies(void) const noexcept {
        return foundation::DependencyBounds{.precedents = {
                                                ShadowMapRenderPassConstructor::NODE,
                                                BackgroundRenderPassConstructor::NODE,
                                            }};
    }

    std::vector<std::tuple<types::ShaderKind, std::filesystem::path>>
    enishi::render_pass::ModelRenderPassConstructor::get_paths(void) const noexcept {
        return {
            {types::ShaderKind::Vertex, "vs_model" + this->backend.shader_suffix},
            {types::ShaderKind::Pixel, "ps_model" + this->backend.shader_suffix},
        };
    }

    foundation::UTF8 enishi::render_pass::ModelRenderPassConstructor::get_render_pass_name(
        void) const noexcept {
        return RENDER_PASS_NAME;
    }
} // namespace enishi::render_pass