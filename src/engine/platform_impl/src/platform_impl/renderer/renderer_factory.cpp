#include "renderer_factory.h"
#include <utility>
#if defined(USE_VULKAN)
#include <platform_impl/window/sdl/sdl3_window.h>
#include <renderer/vulkan/runtime/vulkan_render_initializer.h>
#elif defined(USE_OPENGL40)
#include <renderer/opengl/opengl40/runtime/opengl40_render_initializer.h>
#else
#include <renderer/directx/directx11/d3d11_render_initializer.h>
#endif

namespace enishi::platform_impl {
    std::filesystem::path shader_directory() {
#if defined(ENISHI_GENERATED_SHADER_DIRECTORY)
        return ENISHI_GENERATED_SHADER_DIRECTORY;
#else
        return "./assets/shader";
#endif
    }

    platform::RenderBackendDescription render_backend_description() {
#if defined(USE_VULKAN)
        return {types::GraphicsAPI::Vulkan11, "_vk", types::ImageFormat::D32_FLOAT};
#elif defined(USE_OPENGL40)
        return {types::GraphicsAPI::OpenGL40, "_gl", types::ImageFormat::D24_UNORM_S8_UINT};
#else
        return {types::GraphicsAPI::DirectX11, "", types::ImageFormat::D24_UNORM_S8_UINT};
#endif
    }

    platform::RenderResult<RendererInstance> create_renderer(
        const std::shared_ptr<platform::IWindow>& window, const types::WindowSize& size) {
        if (window == nullptr) {
            return foundation::Error(
                platform::RenderError::MakeError, "Renderer requires a window");
        }
        const auto handle = window->get_handle();
        if (handle.is_none()) {
            return foundation::Error(
                platform::RenderError::MakeError, "Window handle is unavailable");
        }
#if defined(USE_VULKAN)
        const auto sdl_window = std::dynamic_pointer_cast<SDL3Window>(window);
        if (sdl_window == nullptr || sdl_window->get_window_handle().is_none()) {
            return foundation::Error(
                platform::RenderError::MakeError, "Vulkan requires an SDL3 window");
        }
        auto result = renderer::vulkan::VulkanRenderInitializer{}.init(
            sdl_window->get_window_handle().unwrap());
#elif defined(USE_OPENGL40)
        auto context = window->create_opengl_context();
        if (context.is_err()) {
            return std::move(context).unwrap_err();
        }
        auto result =
            renderer::opengl::OpenGL40RenderInitializer{}.init(std::move(context.unwrap_mut()));
#else
        auto result = renderer::directx::D3D11RenderInitializer{true}.init(handle.unwrap(), size);
#endif
        if (result.is_err()) {
            return std::move(result).unwrap_err();
        }
        // The alias keeps the renderer and its resources alive while the encoder is in use.
        auto renderer = std::move(result).unwrap();
        auto encoder = std::shared_ptr<platform::IRenderCommandEncoder>(
            renderer, &renderer->get_command_encoder());
        return RendererInstance{std::move(renderer), std::move(encoder)};
    }
} // namespace enishi::platform_impl
