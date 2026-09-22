#include "shadow_map_render_pass_constructor.h"
#include <render_pass/constructor/pipeline_description_builder.h>

namespace enishi::render_pass {
    constexpr char VS_FILE_NAME[] = "vs_shadow_map";
    constexpr char PS_FILE_NAME[] = "ps_shadow_map";

    foundation::Result<std::shared_ptr<platform::IRenderPass>, ConstructError>
    ShadowMapRenderPassConstructor::make(platform::IRenderer* const renderer,
        const platform::IWindow* window,
        const platform::IShaderDataProvider* shader_data_provider,
        std::span<platform::IRenderPass* const> dependency_render_passes) {
        return PipelineDescriptionConstructer{renderer, window}
            .add_topology(types::PrimitiveTopology::TriangleList)
            ->add_shaders(shader_data_provider, this->get_paths())
            ->add_render_target_view(types::ImageFormat::BGRA8_UNORM)
            ->add_depth_stencil_view(types::ImageFormat::D24_UNORM_S8_UINT)
            ->add_rasterizer_state(types::RasterizerStateDescription{
                .cull_mode = types::CullMode::None,
                .front_face = types::FrontFace::CounterClockwise,
            })
            ->build_render_pass(this);
    }

    foundation::DependencyNode ShadowMapRenderPassConstructor::get_node(void) const noexcept {
        return NODE;
    }

    foundation::DependencyBounds ShadowMapRenderPassConstructor::get_dependencies(
        void) const noexcept {
        return {};
    }

    std::vector<std::tuple<types::ShaderKind, std::filesystem::path>>
    ShadowMapRenderPassConstructor::get_paths(void) const noexcept {
        return {
            {types::ShaderKind::Vertex, VS_FILE_NAME},
            {types::ShaderKind::Pixel, PS_FILE_NAME},
        };
    }

    foundation::UTF8 ShadowMapRenderPassConstructor::get_render_pass_name(void) const noexcept {
        return RENDER_PASS_NAME;
    }
} // namespace enishi::render_pass
