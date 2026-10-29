#include "imgui_backend.h"
#include <backends/imgui_impl_sdl3.h>
#include <imgui.h>
#include <platform_impl/ui/font/ui_font.h>
#include <utility>
#if defined(USE_VULKAN)
#include <backends/imgui_impl_vulkan.h>
#include <debug/log/logger.h>
#include <renderer/vulkan/runtime/vulkan_renderer.h>
#elif defined(USE_OPENGL40)
#include <backends/imgui_impl_opengl3.h>
#else
#include <backends/imgui_impl_dx11.h>
#include <renderer/directx/directx11/d3d11_renderer.h>
#endif

namespace enishi::platform_impl {
    ImGuiBackend::ImGuiBackend(
        std::shared_ptr<SDL3Window> window, std::shared_ptr<platform::IRenderer> renderer)
        : window(std::move(window))
        , renderer(std::move(renderer)) {
    }

    foundation::VoidResult<sub_system::UIError> ImGuiBackend::init(void) {
        this->context = ImGui::GetCurrentContext();
        if (this->context == nullptr || this->window == nullptr || this->renderer == nullptr) {
            return foundation::Error(sub_system::UIError::InitializationFailed,
                "UI requires a context, window and renderer");
        }
        const auto native_window = this->window->get_window_handle();
        if (native_window.is_none()) {
            return foundation::Error(
                sub_system::UIError::InitializationFailed, "UI window handle is unavailable");
        }
#if defined(USE_VULKAN)
        const auto vk_renderer =
            std::dynamic_pointer_cast<renderer::vulkan::VulkanRenderer>(this->renderer);
        if (vk_renderer == nullptr) {
            return foundation::Error(
                sub_system::UIError::InitializationFailed, "UI requires a Vulkan renderer");
        }
        const auto device = vk_renderer->get_native_context();
        this->platform_initialized = ImGui_ImplSDL3_InitForVulkan(native_window.unwrap());
        if (this->platform_initialized) {
            ImGui_ImplVulkan_InitInfo info{};
            info.ApiVersion = VK_API_VERSION_1_1;
            info.Instance = device->instance;
            info.PhysicalDevice = device->physical;
            info.Device = device->device;
            info.QueueFamily = device->queue_family;
            info.Queue = device->queue;
            constexpr std::uint32_t UI_DESCRIPTOR_POOL_SIZE = 64;
            constexpr std::uint32_t MINIMUM_IMAGE_COUNT = 2;
            info.DescriptorPoolSize = UI_DESCRIPTOR_POOL_SIZE;
            info.MinImageCount = MINIMUM_IMAGE_COUNT;
            info.ImageCount = device->image_count;
            info.PipelineInfoMain.RenderPass = device->render_pass;
            info.PipelineInfoMain.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
            info.CheckVkResultFn = [](VkResult code) {
                if (code != VK_SUCCESS) {
                    debug::Logger::error(
                        std::format("ImGui Vulkan failed ({})", static_cast<int>(code)));
                }
            };
            this->renderer_initialized = ImGui_ImplVulkan_Init(&info);
        }
#elif defined(USE_OPENGL40)
        const auto gl_context = SDL_GL_GetCurrentContext();
        if (gl_context == nullptr) {
            return foundation::Error(
                sub_system::UIError::InitializationFailed, "UI requires a current OpenGL context");
        }
        this->platform_initialized =
            ImGui_ImplSDL3_InitForOpenGL(native_window.unwrap(), gl_context);
        if (this->platform_initialized) {
            this->renderer_initialized = ImGui_ImplOpenGL3_Init("#version 400 core");
        }
#else
        const auto d3d_renderer =
            std::dynamic_pointer_cast<renderer::directx::D3D11Renderer>(this->renderer);
        if (d3d_renderer == nullptr) {
            return foundation::Error(
                sub_system::UIError::InitializationFailed, "UI requires a D3D11 renderer");
        }
        const auto native_context = d3d_renderer->get_native_context();
        this->platform_initialized = ImGui_ImplSDL3_InitForD3D(native_window.unwrap());
        if (this->platform_initialized) {
            this->renderer_initialized = ImGui_ImplDX11_Init(
                native_context->get_device().Get(), native_context->get_context().Get());
        }
#endif
        if (!this->renderer_initialized) {
            this->shutdown();
            return foundation::Error(sub_system::UIError::InitializationFailed,
                "ImGui platform/renderer backend initialization failed");
        }
        load_ui_font();
        return {};
    }

    void ImGuiBackend::shutdown(void) noexcept {
        if (this->context == nullptr) {
            return;
        }
        ImGui::SetCurrentContext(this->context);
        if (this->renderer_initialized) {
#if defined(USE_VULKAN)
            const auto vk_renderer =
                std::dynamic_pointer_cast<renderer::vulkan::VulkanRenderer>(this->renderer);
            if (vk_renderer != nullptr) {
                const auto result = vk_renderer->get_native_context()->wait_idle();
                if (result.is_err()) {
                    debug::Logger::error(result.unwrap_err().get_message());
                }
            }
            ImGui_ImplVulkan_Shutdown();
#elif defined(USE_OPENGL40)
            ImGui_ImplOpenGL3_Shutdown();
#else
            ImGui_ImplDX11_Shutdown();
#endif
        }
        if (this->platform_initialized) {
            ImGui_ImplSDL3_Shutdown();
        }
        this->renderer_initialized = false;
        this->platform_initialized = false;
        this->context = nullptr;
    }

    void ImGuiBackend::begin_frame(void) {
#if defined(USE_VULKAN)
        ImGui_ImplVulkan_NewFrame();
#elif defined(USE_OPENGL40)
        ImGui_ImplOpenGL3_NewFrame();
#else
        ImGui_ImplDX11_NewFrame();
#endif
        ImGui_ImplSDL3_NewFrame();
    }

    void ImGuiBackend::render(void) {
#if defined(USE_VULKAN)
        const auto vk_renderer =
            std::dynamic_pointer_cast<renderer::vulkan::VulkanRenderer>(this->renderer);
        if (vk_renderer != nullptr) {
            const auto device = vk_renderer->get_native_context();
            if (device->recording) {
                device->begin_main_pass();
                ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), device->command);
            }
        }
#elif defined(USE_OPENGL40)
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
#else
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
#endif
    }

    void ImGuiBackend::process_event(const SDL_Event& event) {
        if (this->platform_initialized) {
            ImGui::SetCurrentContext(this->context);
            ImGui_ImplSDL3_ProcessEvent(&event);
        }
    }
} // namespace enishi::platform_impl
