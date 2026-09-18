#include "model_render_pass_constructor.h"
#include <foundation/log/logger.h>
#include <foundation/path/path_utility.h>
#include <foundation/str/string_builder.h>
#include <render_pass/constructor/background/background_render_pass_constructor.h>
#include <render_pass/constructor/pipeline_description_builder.h>
#include <render_pass/constructor/shadow/shadow_map_render_pass_constructor.h>
#include <render_pass/render_pass.h>

// #include <platform/window/interface_window.h>

namespace enishi::render_pass {
    constexpr char VS_FILE_NAME[] = "vs_model";
    constexpr char PS_FILE_NAME[] = "ps_model";

    foundation::Result<std::shared_ptr<platform::IRenderPass>, ConstructError>
    enishi::render_pass::ModelRenderPassConstructor::make(platform::IRenderer* const renderer,
        const platform::IWindow* window,
        const platform::IShaderDataProvider* shader_data_provider,
        std::span<platform::IRenderPass* const> dependency_render_passes) {
        return PipelineDescriptionConstructer{renderer, window}
            .add_topology(types::PrimitiveTopology::TriangleList)
            ->add_shaders(shader_data_provider, this->get_paths())
            ->add_swap_chain_render_target_view(types::ImageFormat::BGRA8_UNORM)
            ->add_depth_stencil_view(types::ImageFormat::D24_UNORM_S8_UINT)
            ->add_rasterizer_state(types::RasterizerStateDescription{
                .cull_mode = types::CullMode::None,
                .front_face = types::FrontFace::CounterClockwise,
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
            {types::ShaderKind::Vertex, VS_FILE_NAME},
            {types::ShaderKind::Pixel, PS_FILE_NAME},
        };
    }

    foundation::UTF8 enishi::render_pass::ModelRenderPassConstructor::get_render_pass_name(
        void) const noexcept {
        return RENDER_PASS_NAME;
    }
} // namespace enishi::render_pass