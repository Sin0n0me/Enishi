#pragma once
#include <foundation/result/result.h>
#include <memory>
#include <platform/renderer/interface_renderer.h>
#include <platform/window/interface_window.h>
#include <sub_system/ui/interface_ui.h>

struct ImGuiContext;
struct SDL_Window;
union SDL_Event;
struct ID3D11Device;
struct ID3D11DeviceContext;

namespace enishi::ui {
    enum class UIError { InitializationFailed };

    struct ImGuiContextDeleter {
        void operator()(ImGuiContext* context) const noexcept;
    };

    class UI final : public sub_system::IUI {
      public:
        explicit UI(std::shared_ptr<platform::IWindow> window,
            std::shared_ptr<platform::IRenderer> renderer);
        ~UI(void) noexcept override;
        [[nodiscard]] foundation::VoidResult<UIError> init_opengl(
            SDL_Window* window, void* gl_context);
        [[nodiscard]] foundation::VoidResult<UIError> init_directx(
            SDL_Window* window, ID3D11Device* device, ID3D11DeviceContext* device_context);
        void process_event(const SDL_Event& event);
        void begin_frame(void) override;
        void end_frame(void) override;
        void render(void) override;
        bool begin_window(const char* title) override;
        void end_window(void) override;
        void text(std::string_view value) override;
        bool button(const char* label) override;
        bool input_text(const char* label, std::string& value) override;
        bool wants_capture_mouse(void) const override;
        bool wants_capture_keyboard(void) const override;

      private:
        // Keep native resources alive until backend shutdown.
        std::shared_ptr<platform::IWindow> window;
        std::shared_ptr<platform::IRenderer> renderer;
        std::unique_ptr<ImGuiContext, ImGuiContextDeleter> context;
        bool platform_initialized = false;
        bool renderer_initialized = false;
        bool frame_ready = false;
        [[nodiscard]] foundation::VoidResult<UIError> create_context(void);
        void shutdown(void) noexcept;
    };
} // namespace enishi::ui
