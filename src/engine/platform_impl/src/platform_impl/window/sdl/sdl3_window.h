#pragma once
#include "../../input/sdl/sdl_input.h"
#include <SDL3/SDL.h>
#include <debug/playback/playback_control.h>
#include <engine_types/renderer/graphics_api.h>
#include <engine_types/window/window_types.h>
#include <foundation/option/option.h>
#include <foundation/result/result.h>
#include <functional>
#include <memory>
#include <optional>
#include <platform/window/interface_window.h>
#include <sub_system/ui/interface_ui.h>

namespace enishi::platform_impl {
    struct SDLWindowDeleter {
        void operator()(SDL_Window* const ptr) const;
    };

    using SDLWindowPtr = std::unique_ptr<SDL_Window, SDLWindowDeleter>;

    class SDL3Window : public platform::IWindow {
      private:
        const platform::WindowSystem window_system;
        SDLWindowPtr window;
        SDL3Input input;
        types::WindowSize size;
        types::WindowPosition position;
        bool is_closing;
        bool video_initialized = false;
        bool events_initialized = false;
        std::optional<types::ScreenMousePosition> model_press;
        types::WindowPosition drag_origin{};
        glm::vec2 applied_drag_offset{0};
        bool model_dragging = false;
        bool model_drag_claimed = false;
        std::function<bool(types::ClientMousePosition)> begin_model_drag;
        std::function<void()> end_model_drag;
        std::weak_ptr<sub_system::IUI> ui;
        std::shared_ptr<debug::PlaybackControl> debug_playback;
        std::function<void(const SDL_Event&)> ui_event_handler;
        std::function<bool(types::ClientMousePosition)> interactive_hit_test;

        static SDL_WindowFlags get_flag_from_graphics_api(const types::GraphicsAPI graphics_api);
        static foundation::Option<platform::NativeWindowHandle> get_native_handle(
            SDL_Window* const window, const platform::WindowSystem window_system);

        explicit SDL3Window(void) = delete;

      public:
        explicit SDL3Window(const foundation::UTF8& window_name,
            const types::WindowSize& size,
            const platform::WindowSystem window_system,
            const types::GraphicsAPI graphics_api,
            SDL_WindowFlags extra_flags = 0);

        ~SDL3Window(void) noexcept;

        foundation::Option<SDL_Window*> get_window_handle(void) const;
        void set_debug_playback(std::shared_ptr<debug::PlaybackControl> playback);
        void set_ui(
            std::weak_ptr<sub_system::IUI> ui, std::function<void(const SDL_Event&)> event_handler);

        foundation::VoidResult<platform::WindowError> init(void) noexcept override;
        foundation::Option<const platform::IInput*> get_input(void) const noexcept override;
        foundation::Option<platform::WindowHandle> get_handle(void) const noexcept override;
        foundation::Result<std::shared_ptr<platform::IOpenGLContext>, platform::RenderError>
        create_opengl_context(void) override;
        foundation::Option<types::WindowPosition> get_position(void) const noexcept override;
        foundation::VoidResult<platform::WindowError> set_position(
            const types::WindowPosition& position) noexcept override;
        foundation::Option<types::WindowSize> get_size(void) const noexcept override;
        foundation::VoidResult<platform::WindowError> set_size(
            const types::WindowSize& size) noexcept override;
        foundation::VoidResult<platform::WindowError> set_title(
            const std::string& title) noexcept override;
        foundation::Option<std::string> get_title(void) const noexcept override;
        void close(void) override;
        bool should_close(void) const override;
        void poll_events(void) override;
        void set_model_drag_handler(std::function<bool(types::ClientMousePosition)> begin,
            std::function<void()> end) override;
        void set_interactive_hit_test(
            std::function<bool(types::ClientMousePosition)> hit_test) override;

      private:
        static SDL_HitTestResult SDLCALL hit_test_callback(
            SDL_Window* window, const SDL_Point* point, void* data);
        bool apply_shape_window(void);
        bool handle_model_pointer(const SDL_Event& event, bool capture_mouse);
        void update_model_drag(void);
        void cancel_model_drag(void);
        void refresh_mouse_position(void);
    };
} // namespace enishi::platform_impl
