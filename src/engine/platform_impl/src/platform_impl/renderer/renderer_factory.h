#pragma once
#include <filesystem>
#include <platform/renderer/interface_render_command_encoder.h>
#include <platform/renderer/interface_renderer.h>
#include <platform/renderer/render_backend_description.h>
#include <platform/window/interface_window.h>

namespace enishi::platform_impl {
    struct RendererInstance {
        std::shared_ptr<platform::IRenderer> renderer;
        std::shared_ptr<platform::IRenderCommandEncoder> encoder;
    };
    [[nodiscard]] std::filesystem::path shader_directory();
    [[nodiscard]] platform::RenderBackendDescription render_backend_description();
    [[nodiscard]] platform::RenderResult<RendererInstance> create_renderer(
        const std::shared_ptr<platform::IWindow>& window, const types::WindowSize& size);
} // namespace enishi::platform_impl
