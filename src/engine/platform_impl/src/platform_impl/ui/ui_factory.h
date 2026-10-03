#pragma once
#include <memory>
#include <platform/renderer/interface_renderer.h>
#include <platform/window/interface_window.h>
#include <sub_system/ui/interface_ui.h>
#include <sub_system/ui/interface_ui_backend.h>

namespace enishi::platform_impl {
    [[nodiscard]] foundation::Result<std::shared_ptr<sub_system::IUI>, sub_system::UIError>
    create_ui(
        std::shared_ptr<platform::IWindow> window, std::shared_ptr<platform::IRenderer> renderer);
} // namespace enishi::platform_impl
