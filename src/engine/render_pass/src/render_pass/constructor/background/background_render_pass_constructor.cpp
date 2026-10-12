#include "background_render_pass_constructor.h"
#include <render_pass/constructor/pipeline_description_builder.h>
#include <render_pass/constructor/shadow/shadow_map_render_pass_constructor.h>

namespace enishi::render_pass {
#if defined(USE_VULKAN)
    constexpr char VS_FILE_NAME[] = "vs_clear_wall_vk";
    constexpr char PS_FILE_NAME[] = "ps_clear_wall_vk";
#else
    constexpr char VS_FILE_NAME[] = "vs_clear_wall";
    constexpr char PS_FILE_NAME[] = "ps_clear_wall";
#endif

    foundation::Result<std::shared_ptr<platform::IRenderPass>, ConstructError>
    BackgroundRenderPassConstructor::make(platform::IRenderer* const renderer,
        const platform::IWindow* window,
        const platform::IShaderDataProvider* shader_data_provider,
        std::span<platform::IRenderPass* const> dependency_render_passes) {
        return PipelineDescriptionConstructer{renderer, window}
            .add_topology(types::PrimitiveTopology::TriangleList)
            ->add_swap_chain_render_target_view(types::ImageFormat::BGRA8_UNORM)
            ->add_shaders(shader_data_provider, this->get_paths())
            ->add_depth_stencil_view(
#if defined(USE_VULKAN)
                types::ImageFormat::D32_FLOAT)
#else
                types::ImageFormat::D24_UNORM_S8_UINT)
#endif
            ->add_blend_state(types::BlendStateDescription{
#if defined(USE_VULKAN)
                .alpha_to_coverage = false,
                .independent_blend = false,
#else
                .alpha_to_coverage = true,
                .independent_blend = true,
#endif
                .render_targets = types::RenderTargetBlendState::default_blend_state(),
            })
            ->add_depth_stencil_state(types::DepthStencilStateDescription{
                .depth =
                    types::DepthState{
                        .enabled = true,
                        .write_enabled = false,
                        .compare_operator = types::CompareOperator::LessEqual,
                    },
                .stencil =
                    types::StencilState{
                        .enabled = false,
                        .front = types::StencilFaceState{},
                        .back = types::StencilFaceState{},
                    },
            })
            ->add_rasterizer_state(types::RasterizerStateDescription{
                .cull_mode = types::CullMode::None,
                .front_face = types::FrontFace::CounterClockwise,
            })
            ->build_render_pass(this);
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
    BackgroundRenderPassConstructor::get_paths(void) const noexcept {
        return {
            {types::ShaderKind::Vertex, VS_FILE_NAME},
            {types::ShaderKind::Pixel, PS_FILE_NAME},
        };
    }

    foundation::UTF8 BackgroundRenderPassConstructor::get_render_pass_name(void) const noexcept {
        return RENDER_PASS_NAME;
    }
} // namespace enishi::render_pass
