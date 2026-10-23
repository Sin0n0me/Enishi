#include "background_render_pass_constructor.h"
#include <render_pass/constructor/pipeline_description_builder.h>
#include <render_pass/constructor/shadow/shadow_map_render_pass_constructor.h>
#include <utility>

namespace enishi::render_pass {
    namespace {
        types::ModelData make_shadow_wall() {
            types::ModelData wall;
            // A unit square is transformed into the camera-facing receiver by the render system.
            constexpr float HALF_SIZE = 1.0f;
            for (const auto position : {glm::vec3{-HALF_SIZE, -HALF_SIZE, 0},
                     glm::vec3{HALF_SIZE, -HALF_SIZE, 0},
                     glm::vec3{HALF_SIZE, HALF_SIZE, 0},
                     glm::vec3{-HALF_SIZE, HALF_SIZE, 0}}) {
                wall.vertices.push_back({types::Vertex{position, {0, 0, -1}, {0, 0}},
                    types::Skinning4{{0, 0, 0, 0}, {1, 0, 0, 0}}});
            }
            wall.indices = std::vector<std::uint16_t>{0, 1, 2, 0, 2, 3};
            types::Material material{};
            material.name = "Material";
            constexpr std::uint32_t INDEX_COUNT = 6;
            material.count = INDEX_COUNT;
            material.instance_count = 1;
            material.variants = {types::Diffuse{{0, 0, 0, 1}}};
            wall.materials.push_back(std::move(material));
            return wall;
        }
    } // namespace

    BackgroundRenderPassConstructor::BackgroundRenderPassConstructor(
        platform::RenderBackendDescription backend)
        : backend(std::move(backend)) {
    }

    foundation::Result<std::shared_ptr<platform::IRenderPass>, ConstructError>
    BackgroundRenderPassConstructor::make(platform::IRenderer* const renderer,
        const platform::IWindow* window,
        const platform::IShaderDataProvider* shader_data_provider,
        std::span<platform::IRenderPass* const> dependency_render_passes) {
        types::RenderHandle shadow_depth;
        for (const auto* pass : dependency_render_passes) {
            if (pass->get_name() == ShadowMapRenderPassConstructor::RENDER_PASS_NAME) {
                shadow_depth = pass->get_sampled_depth();
            }
        }
        auto pass = PipelineDescriptionConstructer{renderer, window}
                        .add_topology(types::PrimitiveTopology::TriangleList)
                        ->add_shader_resource(shadow_depth)
                        ->add_swap_chain_render_target_view(types::ImageFormat::BGRA8_UNORM)
                        ->add_shaders(shader_data_provider, this->get_paths())
                        ->add_depth_stencil_view(this->backend.depth_format)
                        ->add_blend_state(types::BlendStateDescription{
                            .alpha_to_coverage = false,
                            .independent_blend = false,
                            .render_targets = types::RenderTargetBlendState::default_blend_state(),
                        })
                        ->add_depth_stencil_state(types::DepthStencilStateDescription{
                            .depth =
                                types::DepthState{
                                    .enabled = false,
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
                            .line_width = 1.0f,
                        })
                        ->build_render_pass(this);
        if (pass.is_err() || !shadow_depth.is_valid()) {
            return pass;
        }
        const auto reflections = pass.unwrap()->get_shader_reflections();
        auto mesh = renderer->create_mesh(make_shadow_wall(),
            std::vector<types::RenderHandle>(reflections.begin(), reflections.end()));
        if (mesh.is_err()) {
            return mesh.propagation(ConstructError::Construct);
        }
        pass.unwrap()->set_shadow_receiver(mesh.unwrap());
        return pass;
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
            {types::ShaderKind::Vertex, "vs_clear_wall" + this->backend.shader_suffix},
            {types::ShaderKind::Pixel, "ps_clear_wall" + this->backend.shader_suffix},
        };
    }

    foundation::UTF8 BackgroundRenderPassConstructor::get_render_pass_name(void) const noexcept {
        return RENDER_PASS_NAME;
    }
} // namespace enishi::render_pass
