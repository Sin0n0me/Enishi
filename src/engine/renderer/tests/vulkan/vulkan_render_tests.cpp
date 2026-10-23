#include "vulkan_render_tests.h"
#include "render_pass_test.h"
#include "shadow_render_test.h"
#include <SDL3/SDL.h>
#include <array>
#include <backends/imgui_impl_sdl3.h>
#include <backends/imgui_impl_vulkan.h>
#include <cmath>
#include <core/system/render/bone_matrix_upload.h>
#include <engine_types/renderer/uniform_buffer/bones.h>
#include <engine_types/renderer/uniform_buffer/camera.h>
#include <engine_types/renderer/uniform_buffer/material.h>
#include <fstream>
#include <glm/gtc/matrix_transform.hpp>
#include <imgui.h>
#include <iostream>
#include <iterator>
#include <renderer/common/converter/skinned_vertices.h>
#include <renderer/common/upload/buffer_upload.h>
#include <renderer/vulkan/runtime/vulkan_render_initializer.h>

using namespace enishi;
namespace {
    constexpr int WIDTH = 256;
    constexpr int HEIGHT = 256;
    constexpr std::size_t CHANNELS = 4;
    constexpr std::size_t MINIMUM_LIT_PIXELS = 1000;
    constexpr std::uint32_t HIGH_BONE_INDEX = 600;
    void require(bool condition, const char* message) {
        if (!condition) {
            std::cerr << message << '\n';
            std::exit(EXIT_FAILURE);
        }
    }
    template <class T> T take(platform::RenderResult<T>&& result) {
        if (result.is_err()) {
            std::cerr << result.unwrap_err().get_message() << '\n';
            std::exit(EXIT_FAILURE);
        }
        return std::move(result.unwrap_mut());
    }
    void success(platform::RenderResult<void>&& result) {
        if (result.is_err()) {
            std::cerr << result.unwrap_err().get_message() << '\n';
            std::exit(EXIT_FAILURE);
        }
    }
    types::ShaderData shader(const char* name) {
        std::ifstream input(
            std::filesystem::path(VULKAN_SHADER_DIRECTORY) / name, std::ios::binary);
        require(input.is_open(), "Generated Vulkan shader missing");
        return {.binary_type = types::ShaderBinaryType::SPIR_V,
            .code = {std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()}};
    }
    types::ModelData triangle(types::SkinningMethod method) {
        types::ModelData model;
        constexpr float HALF = 0.5f;
        constexpr float DEPTH = 0.2f;
        const std::array positions{glm::vec3{-HALF, -HALF, DEPTH},
            glm::vec3{HALF, -HALF, DEPTH},
            glm::vec3{0, HALF, DEPTH}};
        for (const auto position : positions) {
            model.vertices.push_back({types::Vertex{position, {0, 0, -1}, {0, 0}},
                types::Skinning4{{1, 2, 3, 4}, {0.1f, 0.2f, 0.3f, 0.4f}}});
            model.skinning_methods.push_back(method);
            if (method == types::SkinningMethod::SphericalBlend) {
                model.vertices.back()[1] = types::Skinning4{{3, 4, 0, 0}, {0.3f, 0.7f, 0, 0}};
                model.spherical_blends.push_back({});
            }
        }
        model.indices = std::vector<std::uint8_t>{0, 1, 2};
        types::Material material{};
        material.name = "Material";
        material.count = static_cast<std::uint32_t>(positions.size());
        material.instance_count = 1;
        material.variants = {
            types::Diffuse{{0, 0, 0, 1}}, types::Ambient{{0, 1, 0}}, types::Specular{{0, 0, 0}, 1}};
        model.materials.push_back(material);
        return model;
    }
    types::DrawCommand bind(types::RenderHandle handle) {
        return {.handle = handle, .sub_command = types::SubCommand::Bind};
    }
    std::shared_ptr<platform::IResourceUpdater> uniform(
        renderer::vulkan::VulkanRenderer& renderer, types::RenderHandle mesh, const char* name) {
        auto* resources = renderer.get_resource_accessor()->get_resource_accessor();
        const auto mapped = renderer.get_handle_mapper()->get(mesh).unwrap().resource;
        const auto handle = resources->get_mesh_accessor()
                                ->get_mesh_handle(mapped)
                                .unwrap()
                                .uniform_buffers.at(name)
                                .front();
        return resources->get_buffer_accessor()->get_buffer(handle).unwrap();
    }
    double centroid(
        const renderer::vulkan::VulkanBuffer& pixels, std::uint32_t width, std::uint32_t height) {
        const auto data = static_cast<const std::uint8_t*>(pixels.mapped);
        std::size_t count = 0;
        double x_sum = 0;
        constexpr std::uint8_t BRIGHT = 200;
        for (std::uint32_t y = 0; y < height; ++y) {
            for (std::uint32_t x = 0; x < width; ++x) {
                const auto index = (static_cast<std::size_t>(y) * width + x) * CHANNELS;
                if (data[index + 1] > BRIGHT && data[index + 2] < BRIGHT) {
                    ++count;
                    x_sum += x;
                }
            }
        }
        require(count > MINIMUM_LIT_PIXELS, "Model triangle was not rendered");
        // Pixel centroid x = sum(x of lit pixels) / lit pixel count.
        return x_sum / static_cast<double>(count);
    }
    void model_test(renderer::vulkan::VulkanRenderer& renderer,
        types::SkinningMethod method,
        bool large_palette = false) {
        const auto device = renderer.get_native_context();
        auto model = triangle(method);
        if (large_palette) {
            model.addons.emplace_back(types::AddonBones(HIGH_BONE_INDEX + 1));
            for (auto& vertex : model.vertices) {
                std::get<types::Skinning4>(vertex[1]).bone_index.w = HIGH_BONE_INDEX;
            }
        }
        const auto mesh = take(renderer.create_mesh(model, {}));
        const auto first_sampler =
            take(renderer.create_sampler(types::SamplerStateDescription::default_linear()));
        const auto second_sampler =
            take(renderer.create_sampler(types::SamplerStateDescription::default_linear()));
        auto camera = uniform(renderer, mesh, "Camera");
        const types::UniformCamera identity{glm::mat4{1}, glm::mat4{1}, glm::mat4{1}, glm::mat4{1}};
        camera->get_resource().update(identity, 0);
        camera->on_update();
        auto bones = uniform(renderer, mesh, "Bones");
        std::array<glm::mat4, types::HeavyModelBones::CAPACITY> palette;
        palette.fill(glm::mat4{1});
        success(renderer::write_bone_matrices(*bones, palette));
        auto pixels = take(device->make_buffer(
            static_cast<VkDeviceSize>(device->extent.width) * device->extent.height * CHANNELS,
            VK_BUFFER_USAGE_TRANSFER_DST_BIT));
        const auto frame = [&]() {
            renderer.get_command_encoder().setup_views();
            require(device->recording, "Frame not acquired");
            renderer.get_command_encoder().submit_command_state(bind(first_sampler));
            renderer.get_command_encoder().submit_command_mesh(bind(mesh), {});
            renderer.get_command_encoder().submit_command_state(bind(second_sampler));
            renderer.get_command_encoder().submit_command_mesh(bind(mesh), {});
            renderer.get_command_encoder().submit_command_state(bind(first_sampler));
            renderer.get_command_encoder().submit_command_mesh(bind(mesh), {});
            success(device->end_frame(pixels));
            return centroid(*pixels, device->extent.width, device->extent.height);
        };
        const auto before = frame();
        require(std::abs(before - device->extent.width / 2.0) < 2.0,
            "Meshes inherited another mesh's bone/camera data");
        constexpr float TRANSLATION = 0.25f;
        palette[3] = glm::translate(glm::mat4{1}, glm::vec3{TRANSLATION, 0, 0});
        palette[4] = palette[3];
        palette[HIGH_BONE_INDEX] = palette[3];
        if (method == types::SkinningMethod::SphericalBlend) {
            palette[3] = glm::mat4{1};
        }
        success(renderer::write_bone_matrices(*bones, palette));
        const auto after = frame();
        // Two translated influences contribute (0.3 + 0.4) * translation; NDC spans two units.
        const auto expected = 0.7 * TRANSLATION * device->extent.width / 2.0;
        constexpr double PIXEL_TOLERANCE = 1.5;
        require(std::abs(after - before - expected) < PIXEL_TOLERANCE,
            "Four-influence bone upload/skinning did not move pixels correctly");
        auto* resources = renderer.get_resource_accessor()->get_resource_accessor();
        const auto mapped = renderer.get_handle_mapper()->get(mesh).unwrap().resource;
        const auto& mesh_handles = resources->get_mesh_accessor()->get_mesh_handle(mapped).unwrap();
        const auto positions =
            resources->get_buffer_accessor()->get_buffer(mesh_handles.positions->buffer).unwrap();
        auto& vertices = positions->get_resource();
        const auto vertex_data = vertices.get_render_data();
        constexpr float MORPH_TRANSLATION = -0.2f;
        for (std::size_t index = 0; index < 3; ++index) {
            renderer::SkinnedVertex vertex;
            std::memcpy(&vertex, vertex_data.raw_data() + index * sizeof(vertex), sizeof(vertex));
            vertex.position.x += MORPH_TRANSLATION;
            vertices.update(vertex, index);
        }
        positions->on_update();
        const auto morphed = frame();
        const auto expected_morph = MORPH_TRANSLATION * device->extent.width / 2.0;
        require(std::abs(morphed - after - expected_morph) < PIXEL_TOLERANCE,
            "Vertex morph upload did not move pixels");
        std::cout << "Skinning " << static_cast<int>(method) << ": pixel displacement "
                  << after - before << '\n';
    }
    void ui_test(renderer::vulkan::VulkanRenderer& renderer, SDL_Window* window) {
        const auto device = renderer.get_native_context();
        ImGui::CreateContext();
        ImGui::GetIO().IniFilename = nullptr;
        require(ImGui_ImplSDL3_InitForVulkan(window), "ImGui SDL init failed");
        ImGui_ImplVulkan_InitInfo info{};
        info.ApiVersion = VK_API_VERSION_1_1;
        info.Instance = device->instance;
        info.PhysicalDevice = device->physical;
        info.Device = device->device;
        info.QueueFamily = device->queue_family;
        info.Queue = device->queue;
        constexpr std::uint32_t POOL_SIZE = 64;
        info.DescriptorPoolSize = POOL_SIZE;
        info.MinImageCount = 2;
        info.ImageCount = device->image_count;
        info.PipelineInfoMain.RenderPass = device->render_pass;
        info.PipelineInfoMain.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
        require(ImGui_ImplVulkan_Init(&info), "ImGui Vulkan init failed");
        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();
        constexpr float UI_SIZE = 32;
        ImGui::GetBackgroundDrawList()->AddRectFilled(
            {0, 0}, {UI_SIZE, UI_SIZE}, IM_COL32(255, 0, 0, 255));
        ImGui::Render();
        renderer.get_command_encoder().setup_views();
        require(device->recording, "UI frame not acquired");
        device->begin_main_pass();
        ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), device->command);
        auto pixels = take(device->make_buffer(
            static_cast<VkDeviceSize>(device->extent.width) * device->extent.height * CHANNELS,
            VK_BUFFER_USAGE_TRANSFER_DST_BIT));
        success(device->end_frame(pixels));
        const auto data = static_cast<const std::uint8_t*>(pixels->mapped);
        constexpr std::size_t SAMPLE = 8;
        const auto index = (SAMPLE * device->extent.width + SAMPLE) * CHANNELS;
        require(data[index + 2] > 200 && data[index + 1] < 20, "ImGui pixels missing");
        success(device->wait_idle());
        ImGui_ImplVulkan_Shutdown();
        ImGui_ImplSDL3_Shutdown();
        ImGui::DestroyContext();
    }
    void resize_test(renderer::vulkan::VulkanRenderer& renderer, SDL_Window* window) {
        constexpr int RESIZED_WIDTH = 320;
        constexpr int MAXIMUM_FRAMES = 3;
        require(SDL_SetWindowSize(window, RESIZED_WIDTH, HEIGHT), SDL_GetError());
        SDL_PumpEvents();
        const auto device = renderer.get_native_context();
        for (int frame = 0; frame < MAXIMUM_FRAMES; ++frame) {
            renderer.get_command_encoder().setup_views();
            success(device->end_frame());
        }
        require(device->extent.width == RESIZED_WIDTH && device->extent.height == HEIGHT,
            "Swapchain did not follow window resize");
    }
} // namespace
int enishi::renderer::tests::run_vulkan_render_tests() {
    require(SDL_Init(SDL_INIT_VIDEO), SDL_GetError());
    const auto window = std::unique_ptr<SDL_Window, decltype(&SDL_DestroyWindow)>(
        SDL_CreateWindow(
            "Vulkan rendering validation", WIDTH, HEIGHT, SDL_WINDOW_VULKAN | SDL_WINDOW_HIDDEN),
        SDL_DestroyWindow);
    require(window != nullptr, SDL_GetError());
    auto renderer = take(renderer::vulkan::VulkanRenderInitializer{}.init(window.get(), true));
    require(render_pass_test(renderer), "Application render-pass construction failed");
    const auto vertex = shader("vs_model_vk.spv");
    const auto fragment = shader("ps_model_vk.spv");
    take(renderer->create_shader_reflection(vertex));
    take(renderer->create_shader_reflection(fragment));
    take(renderer->create_vertex_layout_from_shader_data(vertex));
    renderer->get_command_encoder().submit_command_shader(
        bind(take(renderer->create_shader(types::ShaderKind::Vertex, vertex))));
    renderer->get_command_encoder().submit_command_shader(
        bind(take(renderer->create_shader(types::ShaderKind::Pixel, fragment))));
    renderer->get_command_encoder().submit_command_state(
        bind(take(renderer->create_rasterizer(types::RasterizerStateDescription::no_cull()))));
    require(renderer->create_shader(types::ShaderKind::Pixel, vertex).is_err(),
        "Shader stage mismatch was accepted");
    model_test(*renderer, types::SkinningMethod::LinearBlend);
    model_test(*renderer, types::SkinningMethod::DualQuaternion);
    model_test(*renderer, types::SkinningMethod::SphericalBlend);
    model_test(*renderer, types::SkinningMethod::LinearBlend, true);
    resize_test(*renderer, window.get());
    model_test(*renderer, types::SkinningMethod::LinearBlend);
    ui_test(*renderer, window.get());
    shadow_render_test(*renderer);
    auto device = renderer->get_native_context();
    renderer.reset();
    require(device->validation_error_count() == 0, "Vulkan validation reported errors");
    device.reset();
    std::cout << "Vulkan model, bone update and UI rendering passed without validation errors\n";
    return EXIT_SUCCESS;
}

int main() {
    return enishi::renderer::tests::run_vulkan_render_tests();
}
