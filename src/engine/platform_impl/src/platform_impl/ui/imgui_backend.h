#pragma once
#include <memory>
#include <platform/renderer/interface_renderer.h>
#include <platform_impl/window/sdl/sdl3_window.h>
#include <sub_system/ui/interface_ui_backend.h>

struct ImGuiContext;

namespace enishi::platform_impl {
    class ImGuiBackend final : public sub_system::IUIBackend {
      private:
        // Backend shutdown runs before the UI destroys its context.
        ImGuiContext* context = nullptr;
        std::shared_ptr<SDL3Window> window;
        std::shared_ptr<platform::IRenderer> renderer;
        bool platform_initialized = false;
        bool renderer_initialized = false;

      public:
        explicit ImGuiBackend(
            std::shared_ptr<SDL3Window> window, std::shared_ptr<platform::IRenderer> renderer);
        foundation::VoidResult<sub_system::UIError> init(void) override;
        void shutdown(void) noexcept override;
        void begin_frame(void) override;
        void render(void) override;
        void process_event(const SDL_Event& event);
    };
} // namespace enishi::platform_impl
