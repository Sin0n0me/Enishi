#include <array>
#include <core/system/asset/asset_system.h>
#include <core/system/asset/shader/shader_data_provider.h>
#include <core/system/render/bone_matrix_upload.h>
#include <cstdlib>
#include <cstring>
#include <engine_types/renderer/uniform_buffer/camera.h>
#include <engine_types/renderer/uniform_buffer/material.h>
#include <engine_types/renderer/uniform_buffer/shadow.h>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <render_pass/constructor/shadow/shadow_map_render_pass_constructor.h>
#include <renderer/vulkan/vulkan_renderer.h>

using namespace enishi;
namespace {
    constexpr std::size_t CHANNELS = 4;
    constexpr std::uint8_t DARK_LIMIT = 80;
    constexpr std::uint8_t LIT_LIMIT = 180;
    void require(bool condition, const char* message) {
        if (!condition) {
            std::cerr << message << std::endl;
            std::exit(EXIT_FAILURE);
        }
    }
    template <class Result> auto take(Result&& result) {
        if (result.is_err()) {
            std::cerr << result.unwrap_err().get_message() << std::endl;
            std::exit(EXIT_FAILURE);
        }
        return std::move(result.unwrap_mut());
    }
    void success(platform::RenderResult<void>&& result) {
        require(result.is_ok(), result.is_err() ? result.unwrap_err().get_message().c_str() : "");
    }
    types::DrawCommand bind(types::RenderHandle handle) {
        return {.handle = handle, .sub_command = types::SubCommand::Bind};
    }
    std::shared_ptr<platform::IResourceUpdater> uniform(renderer::vulkan::VulkanRenderer& renderer,
        types::RenderHandle mesh,
        const char* name,
        bool material = false) {
        auto* resources = renderer.get_resource_accessor()->get_resource_accessor();
        const auto mapped = renderer.get_handle_mapper()->get(mesh).unwrap().resource;
        const auto& handles = resources->get_mesh_accessor()->get_mesh_handle(mapped).unwrap();
        const auto& uniforms =
            material ? handles.material_uniform_buffers.front() : handles.uniform_buffers;
        return resources->get_buffer_accessor()->get_buffer(uniforms.at(name).front()).unwrap();
    }
    types::ModelData quad(bool caster, types::SkinningMethod method) {
        constexpr float HALF_WIDTH = 0.4f;
        constexpr float BOTTOM = 0.25f;
        constexpr float TOP = 0.75f;
        constexpr float RECEIVER_HALF = 0.9f;
        constexpr float CASTER_DEPTH = 0.25f;
        constexpr float RECEIVER_DEPTH = 0.5f;
        const float left = caster ? -HALF_WIDTH : -RECEIVER_HALF;
        const float right = -left;
        const float bottom = caster ? BOTTOM : -RECEIVER_HALF;
        const float top = caster ? TOP : RECEIVER_HALF;
        const float depth = caster ? CASTER_DEPTH : RECEIVER_DEPTH;
        types::ModelData model;
        const std::array positions{glm::vec3{left, bottom, depth},
            glm::vec3{right, bottom, depth},
            glm::vec3{right, top, depth},
            glm::vec3{left, top, depth}};
        const auto normal = glm::normalize(glm::vec3{-0.35f, 0.8f, -1.0f});
        for (const auto position : positions) {
            model.vertices.push_back({types::Vertex{position, normal, {0, 0}},
                types::Skinning4{{0, 1, 0, 0}, {0.5f, 0.5f, 0, 0}}});
            model.skinning_methods.push_back(method);
            if (method == types::SkinningMethod::SphericalBlend) {
                model.spherical_blends.push_back({});
            }
        }
        model.indices = std::vector<std::uint16_t>{0, 1, 2, 0, 2, 3};
        model.addons.emplace_back(types::AddonBones(2));
        types::Material material{};
        material.name = "Material";
        material.count = 6;
        material.instance_count = 1;
        material.variants = {types::Diffuse{{0.6f, 0.6f, 0.6f, 1}},
            types::Ambient{{0.2f, 0.2f, 0.2f}},
            types::Specular{{0, 0, 0}, 1}};
        model.materials.push_back(material);
        return model;
    }
    types::RenderHandle make_mesh(
        renderer::vulkan::VulkanRenderer& renderer, bool caster, types::SkinningMethod method) {
        const auto mesh = take(renderer.create_mesh(quad(caster, method), {}));
        const types::UniformCamera camera{glm::mat4{1}, glm::mat4{1}, glm::mat4{1}, glm::mat4{1}};
        auto buffer = uniform(renderer, mesh, "Camera");
        buffer->get_resource().update(camera, 0);
        buffer->on_update();
        buffer = uniform(renderer, mesh, "ShadowMap");
        buffer->get_resource().update(types::UniformShadow{glm::mat4{1}}, 0);
        buffer->on_update();
        return mesh;
    }
    void submit(renderer::vulkan::VulkanRenderer& renderer, const platform::IRenderPass& pass) {
        for (const auto& command : pass.get_commands()) {
            switch (command.handle.type) {
                case types::RenderHandleType::View:
                    renderer.submit_command_view(command, pass.get_render_target());
                    break;
                case types::RenderHandleType::State:
                    renderer.submit_command_state(command);
                    break;
                case types::RenderHandleType::Shader:
                    renderer.submit_command_shader(command);
                    break;
                case types::RenderHandleType::Topology:
                    renderer.submit_command_topology(command);
                    break;
                case types::RenderHandleType::VertexLayout:
                    renderer.submit_command_vertex_layout(command);
                    break;
                default:
                    break;
            }
        }
    }
    std::vector<types::RenderHandle> model_shaders(renderer::vulkan::VulkanRenderer& renderer,
        core::ShaderDataProvider& provider,
        bool wall = false) {
        const std::vector<std::tuple<types::ShaderKind, std::filesystem::path>> paths{
            {types::ShaderKind::Vertex, wall ? "vs_clear_wall_vk" : "vs_model_vk"},
            {types::ShaderKind::Pixel, wall ? "ps_clear_wall_vk" : "ps_model_vk"}};
        std::vector<types::RenderHandle> shaders;
        for (const auto& entry : take(provider.get(paths))) {
            take(renderer.create_shader_reflection(*entry.data));
            shaders.push_back(take(renderer.create_shader(entry.kind, *entry.data)));
        }
        return shaders;
    }
    void check_pixels(const renderer::vulkan::VulkanBuffer& pixels,
        VkExtent2D size,
        bool shadowed,
        bool narrow_viewport,
        bool wall) {
        const auto data = static_cast<const std::uint8_t*>(pixels.mapped);
        // Pixel coordinates = ((ndc.x+1)/2*width, (1-ndc.y)/2*height).
        const auto sample_x = narrow_viewport ? size.width / 4 : size.width / 2;
        const auto upper = ((size.height / 4) * size.width + sample_x) * CHANNELS;
        const auto lower = ((size.height * 3 / 4) * size.width + sample_x) * CHANNELS;
        if (wall) {
            require(data[lower + 3] == 0, "unshadowed wall must be transparent");
            require(
                shadowed ? (data[upper + 3] > 60 && data[upper + 3] < 68) : data[upper + 3] == 0,
                "wall shadow alpha or clearing is wrong");
            require(data[upper] == 0 && data[upper + 1] == 0 && data[upper + 2] == 0,
                "wall must only draw black shadow");
            return;
        }
        require(
            data[lower] > LIT_LIMIT, "Shadow image Y orientation or receiver viewport is wrong");
        require(shadowed ? data[upper] < DARK_LIMIT : data[upper] > LIT_LIMIT,
            "Vulkan shadow depth/alpha/skinning or clearing is wrong");
        require(data[upper + 3] > LIT_LIMIT, "Shadow must preserve model alpha");
        if (narrow_viewport) {
            const auto outside = ((size.height / 2) * size.width + size.width * 3 / 4) * CHANNELS;
            require(data[outside + 3] == 0, "Deferred main pass lost the explicit viewport");
        }
    }
    void run_case(renderer::vulkan::VulkanRenderer& renderer,
        const platform::IRenderPass& pass,
        const std::vector<types::RenderHandle>& shaders,
        types::SkinningMethod method,
        bool wall = false) {
        const auto device = renderer.get_native_context();
        const auto caster = make_mesh(renderer, true, method);
        const auto receiver = make_mesh(renderer, false, types::SkinningMethod::LinearBlend);
        auto pixels = take(device->make_buffer(
            static_cast<VkDeviceSize>(device->extent.width) * device->extent.height * CHANNELS,
            VK_BUFFER_USAGE_TRANSFER_DST_BIT));
        const auto viewport = take(renderer.create_viewport({0,
            0,
            static_cast<float>(device->extent.width) * 0.5f,
            static_cast<float>(device->extent.height),
            0,
            1}));
        const auto frame = [&](bool draw_caster,
                               bool expected_shadow,
                               bool with_shadow = true,
                               bool narrow_viewport = false) {
            renderer.setup_views();
            require(device->recording, "Shadow test frame not acquired");
            if (narrow_viewport) {
                renderer.submit_command_viewport(bind(viewport));
            }
            if (with_shadow) {
                submit(renderer, pass);
                if (draw_caster) {
                    renderer.submit_command_mesh(bind(caster), {});
                }
                renderer.submit_command_view(bind(pass.get_sampled_depth()), {});
            }
            for (const auto shader : shaders) {
                renderer.submit_command_shader(bind(shader));
            }
            renderer.submit_command_mesh(bind(receiver), {});
            success(device->end_frame(pixels));
            check_pixels(*pixels, device->extent, expected_shadow, narrow_viewport, wall);
        };
        frame(true, true);
        const auto material = uniform(renderer, caster, "Material", true);
        types::UniformMaterial value{};
        const auto bytes = material->get_resource().get_render_data();
        std::memcpy(&value, bytes.raw_data(), sizeof(value));
        value.diffuse.a = 0;
        material->get_resource().update(value, 0);
        material->on_update();
        frame(true, false);
        value.diffuse.a = 1;
        material->get_resource().update(value, 0);
        material->on_update();
        constexpr float OUTSIDE_LIGHT_VIEW = 2.0f;
        const auto moved = glm::translate(glm::mat4{1}, glm::vec3{OUTSIDE_LIGHT_VIEW, 0, 0});
        const std::array palette{moved, moved};
        success(core::write_bone_matrices(*uniform(renderer, caster, "Bones"), palette));
        frame(true, false);
        const std::array restored{glm::mat4{1}, glm::mat4{1}};
        success(core::write_bone_matrices(*uniform(renderer, caster, "Bones"), restored));
        frame(true, true);
        frame(false, false); // Hidden casters must not leave the previous frame's depth.
        frame(false, false, false);
        frame(
            false, false, false, true); // Rendering without a shadow pass uses an all-lit fallback.
    }
} // namespace
void shadow_render_test(renderer::vulkan::VulkanRenderer& renderer) {
    core::AssetSystem assets;
    core::ShaderDataProvider provider(assets.get_asset_system());
    const auto pass =
        take(render_pass::ShadowMapRenderPassConstructor{}.make(&renderer, nullptr, &provider, {}));
    const auto shaders = model_shaders(renderer, provider);
    for (const auto method : {types::SkinningMethod::LinearBlend,
             types::SkinningMethod::DualQuaternion,
             types::SkinningMethod::SphericalBlend}) {
        run_case(renderer, *pass, shaders, method);
    }
    run_case(renderer,
        *pass,
        model_shaders(renderer, provider, true),
        types::SkinningMethod::LinearBlend,
        true);
    require(renderer.get_native_context()->validation_error_count() == 0,
        "Vulkan validation reported errors in shadow passes");
    std::cout
        << "Vulkan shadow pixels, alpha, all skinning modes, hidden casters and fallback passed"
        << std::endl;
}
