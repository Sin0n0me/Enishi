#include "ui_factory.h"
#include "imgui/imgui_backend.h"
#include <ui/ui.h>
#include <utility>

namespace enishi::platform_impl {
    foundation::Result<std::shared_ptr<sub_system::IUI>, sub_system::UIError> create_ui(
        std::shared_ptr<platform::IWindow> window, std::shared_ptr<platform::IRenderer> renderer) {
        const auto sdl_window = std::dynamic_pointer_cast<SDL3Window>(window);
        if (sdl_window == nullptr) {
            return foundation::Error(
                sub_system::UIError::InitializationFailed, "UI requires an SDL3 window");
        }
        const auto backend = std::make_shared<ImGuiBackend>(sdl_window, std::move(renderer));
        const auto ui = std::make_shared<ui::UI>(backend);
        auto result = ui->init();
        if (result.is_err()) {
            return std::move(result).unwrap_err();
        }
        sdl_window->set_ui(
            ui, [weak_backend = std::weak_ptr<ImGuiBackend>(backend)](const SDL_Event& event) {
                if (const auto active_backend = weak_backend.lock(); active_backend != nullptr) {
                    active_backend->process_event(event);
                }
            });
        return ui;
    }
} // namespace enishi::platform_impl
