#include "shadow_map_render_pass_constructor.h"
#include <render_pass/constructor/pipeline_description_builder.h>

namespace enishi::render_pass {
#if defined(USE_OPENGL40)
    constexpr char VS_FILE_NAME[] = "vs_shadow_map_gl";
    constexpr char PS_FILE_NAME[] = "ps_shadow_map_gl";
#else
    constexpr char VS_FILE_NAME[] = "vs_shadow_map";
    constexpr char PS_FILE_NAME[] = "ps_shadow_map";
#endif

    foundation::Result<std::shared_ptr<platform::IRenderPass>, ConstructError>
    ShadowMapRenderPassConstructor::make(platform::IRenderer* const renderer,
        const platform::IWindow* window,
        const platform::IShaderDataProvider* shader_data_provider,
        std::span<platform::IRenderPass* const> dependency_render_passes) {
        constexpr int SHADOW_RESOLUTION = 2048;
        constexpr std::uint32_t SHADOW_TEXTURE_SLOT = 3;
        constexpr float RASTERIZER_LINE_WIDTH = 1.0f;
        const auto image = renderer->create_image(
            types::ImageDescription::make_default(glm::ivec2(SHADOW_RESOLUTION),
                types::ImageFormat::D32_FLOAT,
                types::ImageUsage::DepthStencil | types::ImageUsage::ShaderResource));
        if (image.is_err()) {
            return image.propagation(ConstructError::Construct);
        }
        const auto depth = renderer->create_depth_stencil_view(image.unwrap(),
            types::ImageViewDescription::make_depth_stencil_view_description(
                types::ImageFormat::D32_FLOAT));
        if (depth.is_err()) {
            return depth.propagation(ConstructError::Construct);
        }
        auto sample_description =
            types::ImageViewDescription::make_shader_resource_view_description(
                types::ImageFormat::D32_FLOAT, 0);
        sample_description.aspect = types::ImageAspect::Depth;
        sample_description.shader_resource_slot = SHADOW_TEXTURE_SLOT;
        const auto sampled =
            renderer->create_shader_resource_view(image.unwrap(), sample_description);
        if (sampled.is_err()) {
            return sampled.propagation(ConstructError::Construct);
        }
        return PipelineDescriptionConstructer{renderer, window}
            .add_topology(types::PrimitiveTopology::TriangleList)
            ->add_shaders(shader_data_provider, this->get_paths())
            ->add_depth_stencil_view(depth.unwrap()->get_handle())
            ->set_sampled_depth(sampled.unwrap()->get_handle())
            ->add_depth_stencil_state(types::DepthStencilStateDescription{
                .depth = {.enabled = true,
                    .write_enabled = true,
                    .compare_operator = types::CompareOperator::LessEqual},
                .stencil = {.enabled = false},
            })
            ->add_rasterizer_state(types::RasterizerStateDescription{
                .cull_mode = types::CullMode::None,
                .front_face = types::FrontFace::CounterClockwise,
                .line_width = RASTERIZER_LINE_WIDTH,
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
