#include "crack_pass.h"
#include <cstddef>
#include <cstring>
#include <engine_types/renderer/texture/model_texture.h>
#include <render_pass/constructor/pipeline_description_builder.h>
namespace enishi::render_pass {
    namespace {
        constexpr char UNIFORM_NAME[] = "Crack";
        constexpr char MASK_PATH[] = "generated-crack-mask";
        struct CrackUniform {
            glm::vec4 view_depth_opacity;
            glm::vec4 texel_size;
            glm::vec4 mask_transform;
        };
        static_assert(sizeof(CrackUniform) == 3 * sizeof(glm::vec4));
        static_assert(offsetof(CrackUniform, texel_size) == sizeof(glm::vec4));
        CrackUniform make_uniform(const CrackPass::Settings& settings,
            float opacity,
            glm::vec4 mask_transform = {1, 1, 0, 0}) {
            // Texture-space finite differences use a one-texel displacement, 1 / resolution.
            const float texel = 1.0f / static_cast<float>(settings.mask_size);
            return {{settings.view_x, settings.view_y, settings.depth, opacity},
                {texel, texel, 0, 0},
                mask_transform};
        }
        types::ModelData make_quad(
            const CrackPass::Settings& settings, std::shared_ptr<types::TextureData> mask) {
            types::ModelData model;
            constexpr float CLIP_EDGE = 1.0f;
            for (const auto position : {glm::vec3{-CLIP_EDGE, -CLIP_EDGE, 0},
                     glm::vec3{CLIP_EDGE, -CLIP_EDGE, 0},
                     glm::vec3{CLIP_EDGE, CLIP_EDGE, 0},
                     glm::vec3{-CLIP_EDGE, CLIP_EDGE, 0}}) {
                model.vertices.push_back({types::Vertex{position, {0, 0, 1}, {0, 0}},
                    types::Skinning4{{0, 0, 0, 0}, {1, 0, 0, 0}}});
            }
            model.indices = std::vector<std::uint16_t>{0, 1, 2, 0, 2, 3};
            types::Material material{};
            material.name = UNIFORM_NAME;
            constexpr std::uint32_t QUAD_INDEX_COUNT = 6;
            material.count = QUAD_INDEX_COUNT;
            material.instance_count = 1;
            const auto uniform = make_uniform(settings, settings.opacity);
            material.variants = {
                uniform.view_depth_opacity, uniform.texel_size, uniform.mask_transform};
            // The shared Vulkan descriptor layout requires all three texture slots to be bound.
            material.textures = {{MASK_PATH,
                                     types::ModelTexture::MODEL_TEXTURE_NAME,
                                     types::ModelTexture::MODEL_SAMPLER_NAME},
                {MASK_PATH,
                    types::ModelTexture::SPHERE_TEXTURE_NAME,
                    types::ModelTexture::SPHERE_SAMPLER_NAME},
                {MASK_PATH,
                    types::ModelTexture::TOON_TEXTURE_NAME,
                    types::ModelTexture::TOON_SAMPLER_NAME}};
            model.materials.push_back(std::move(material));
            model.textures.emplace(MASK_PATH, std::move(mask));
            return model;
        }
    } // namespace
    CrackPass::CrackPass(std::shared_ptr<platform::IRenderer> renderer,
        std::shared_ptr<platform::IRenderCommandEncoder> encoder,
        Settings settings)
        : renderer(std::move(renderer))
        , encoder(std::move(encoder))
        , settings(settings) {
    }
    platform::RenderResult<void> CrackPass::initialize(const platform::IWindow& window,
        const platform::IShaderDataProvider& shaders,
        const platform::RenderBackendDescription& backend,
        std::shared_ptr<types::TextureData> mask) {
        const std::vector<std::tuple<types::ShaderKind, std::filesystem::path>> paths{
            {types::ShaderKind::Vertex, "vs_crack" + backend.shader_suffix},
            {types::ShaderKind::Pixel, "ps_crack" + backend.shader_suffix}};
        auto sampler = types::SamplerStateDescription::clamp_linear();
        sampler.anisotropy = types::AnisotropyLevel::None;
        auto description =
            render_pass::PipelineDescriptionConstructer{this->renderer.get(), &window}
                .add_topology(types::PrimitiveTopology::TriangleList)
                ->add_swap_chain_render_target_view(types::ImageFormat::BGRA8_UNORM)
                ->add_shaders(&shaders, paths)
                ->add_depth_stencil_view(backend.depth_format)
                ->add_blend_state(types::BlendStateDescription{.alpha_to_coverage = false,
                    .independent_blend = false,
                    .render_targets = types::RenderTargetBlendState::default_blend_state()})
                ->add_depth_stencil_state(types::DepthStencilStateDescription{
                    .depth = {.enabled = false,
                        .write_enabled = false,
                        .compare_operator = types::CompareOperator::Always},
                    .stencil = {.enabled = false, .front = {}, .back = {}}})
                ->add_rasterizer_state(
                    types::RasterizerStateDescription{.cull_mode = types::CullMode::None,
                        .front_face = types::FrontFace::CounterClockwise,
                        .line_width = 1.0f})
                ->add_sampler_state(std::move(sampler))
                ->build();
        if (description.is_err()) {
            return description.propagation(platform::RenderError::MakeError);
        }
        auto built = this->pass.make_from_description(description.unwrap(), "Crack", {}, {});
        if (built.is_err()) {
            return built;
        }
        const auto reflections = this->pass.get_shader_reflections();
        auto mesh = this->renderer->create_mesh(
            make_quad(this->settings, std::move(mask)), {reflections.begin(), reflections.end()});
        if (mesh.is_err()) {
            return std::move(mesh).take_err();
        }
        this->mesh = mesh.unwrap();
        this->pass.add_mesh(this->mesh);
        return {};
    }
    platform::RenderResult<void> CrackPass::update_uniform(
        float opacity, glm::vec4 mask_transform) {
        const auto mapped = this->renderer->get_handle_mapper()->get(this->mesh);
        if (mapped.is_none()) {
            return foundation::Error(platform::RenderError::ResolveError, "Crack mesh is missing");
        }
        auto* resources = this->renderer->get_resource_accessor()->get_resource_accessor();
        const auto handles =
            resources->get_mesh_accessor()->get_mesh_handle(mapped.unwrap().resource);
        if (handles.is_none() || handles.unwrap().material_uniform_buffers.size() != 1) {
            return foundation::Error(
                platform::RenderError::ResolveError, "Crack material is missing");
        }
        const auto& uniforms = handles.unwrap().material_uniform_buffers.front();
        const auto found = uniforms.find(UNIFORM_NAME);
        if (found == uniforms.end() || found->second.empty()) {
            return foundation::Error(
                platform::RenderError::ResolveError, "Crack uniform is missing");
        }
        const auto uniform = make_uniform(this->settings, opacity, mask_transform);
        for (const auto handle : found->second) {
            auto updater = resources->get_buffer_accessor()->get_buffer(handle);
            if (updater.is_none() || updater.unwrap() == nullptr ||
                updater.unwrap()->get_resource().get_render_data().byte_width() < sizeof(uniform)) {
                return foundation::Error(
                    platform::RenderError::ResolveError, "Crack uniform buffer is invalid");
            }
            std::memcpy(&updater.unwrap()->get_resource()[0], &uniform, sizeof(uniform));
            updater.unwrap()->on_update();
        }
        return {};
    }
    platform::RenderResult<void> CrackPass::draw(float opacity, glm::vec4 mask_transform) {
        auto updated = this->update_uniform(opacity, mask_transform);
        if (updated.is_err()) {
            return updated;
        }
        this->submit();
        this->encoder->present();
        return {};
    }
    void CrackPass::submit() {
        const auto& encoder = this->encoder;
        encoder->setup_viewports();
        encoder->setup_views();
        const auto target = this->pass.get_render_target();
        for (const auto& command : this->pass.get_commands()) {
            switch (command.handle.type) {
                case types::RenderHandleType::Buffer: {
                    encoder->submit_command_buffer(command);
                    break;
                }
                case types::RenderHandleType::Shader: {
                    encoder->submit_command_shader(command);
                    break;
                }
                case types::RenderHandleType::Mesh: {
                    encoder->submit_command_mesh(command, target);
                    break;
                }
                case types::RenderHandleType::View: {
                    encoder->submit_command_view(command, target);
                    break;
                }
                case types::RenderHandleType::ViewPort: {
                    encoder->submit_command_viewport(command);
                    break;
                }
                case types::RenderHandleType::State: {
                    encoder->submit_command_state(command);
                    break;
                }
                case types::RenderHandleType::Image: {
                    encoder->submit_command_image(command);
                    break;
                }
                case types::RenderHandleType::Topology: {
                    encoder->submit_command_topology(command);
                    break;
                }
                case types::RenderHandleType::VertexLayout: {
                    encoder->submit_command_vertex_layout(command);
                    break;
                }
                case types::RenderHandleType::Draw: {
                    encoder->draw(command.handle);
                    break;
                }
                default: {
                    break;
                }
            }
        }
    }
} // namespace enishi::render_pass
