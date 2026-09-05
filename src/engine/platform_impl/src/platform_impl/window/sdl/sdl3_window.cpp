#include "sdl3_window.h"
#include <SDL3/SDL_init.h>
#include <foundation/log/logger.h>
#include <platform/window/common_settings.h>

namespace enishi::platform_impl {
    SDL_HitTestResult SDLCALL hit_test_callback(SDL_Window* win, const SDL_Point* pt, void* data) {
        return SDL_HITTEST_DRAGGABLE;
    }

    void SDLWindowDeleter::operator()(SDL_Window* const ptr) const {
        if (ptr) {
            SDL_DestroyWindow(ptr);
        }
    }

    SDL3Window::SDL3Window(const foundation::UTF8& window_name,
        const types::WindowSize& size,
        const platform::WindowSystem window_system,
        const types::GraphicsAPI graphics_api)
        : window_system(window_system)
        , window(SDLWindowPtr(SDL_CreateWindow(window_name.c_str(),
              size.width,
              size.height,
              SDL_WINDOW_ALWAYS_ON_TOP | SDL_WINDOW_BORDERLESS |
                  SDL3Window::get_flag_from_graphics_api(graphics_api))))
        , is_closing(false)
        , size()
        , position() {
    }

    SDL3Window::~SDL3Window(void) noexcept {
        // TODO: ルートウィンドウの時だけ
        SDL_QuitSubSystem(SDL_INIT_EVENTS);
        SDL_QuitSubSystem(SDL_INIT_VIDEO);
    }

    foundation::VoidResult<platform::WindowError> SDL3Window::init(void) noexcept {
        // TODO: ルートウィンドウの時だけ
        if (!SDL_InitSubSystem(SDL_INIT_VIDEO)) {
            foundation::Logger::error(SDL_GetError());
            return foundation::Error(platform::WindowError::InitError);
        }

        if (!SDL_InitSubSystem(SDL_INIT_EVENTS)) {
            foundation::Logger::error(SDL_GetError());
            return foundation::Error(platform::WindowError::InitError);
        }

        // ヒットテストのコールバックを登録
        if (!SDL_SetWindowHitTest(this->window.get(), hit_test_callback, nullptr)) {
            return foundation::Error(platform::WindowError::InitError);
        }

        return {};
    }

    foundation::Option<const platform::IInput*> SDL3Window::get_input(void) const noexcept {
        return &this->input;
    }

    foundation::Option<SDL_Window*> SDL3Window::get_window_handle(void) const {
        if (!bool(this->window)) {
            return {};
        }
        return this->window.get();
    }

    SDL_WindowFlags SDL3Window::get_flag_from_graphics_api(const types::GraphicsAPI graphics_api) {
        switch (graphics_api) {
            case types::GraphicsAPI::Vulkan10:
            case types::GraphicsAPI::Vulkan11:
            case types::GraphicsAPI::Vulkan12:
            case types::GraphicsAPI::Vulkan13:
            case types::GraphicsAPI::Vulkan14:
                return SDL_WINDOW_VULKAN;
            case types::GraphicsAPI::OpenGL40:
            case types::GraphicsAPI::OpenGL41:
            case types::GraphicsAPI::OpenGL42:
            case types::GraphicsAPI::OpenGL43:
            case types::GraphicsAPI::OpenGL44:
            case types::GraphicsAPI::OpenGL45:
                return SDL_WINDOW_OPENGL;
            default:
                break;
        }
        return 0;
    }

    foundation::Option<platform::NativeWindowHandle> SDL3Window::get_native_handle(
        SDL_Window* const window, const platform::WindowSystem window_system) {
        switch (window_system) {
            case platform::WindowSystem::Wayland: {
#if defined(USE_WAYLAND)
                const auto handle = platform::NativeWaylandHandle{};
#else
                const auto handle = platform::NativeWaylandHandle{};
#endif

                return platform::NativeWindowHandle{
                    .wayland = handle,
                };
            }
            case platform::WindowSystem::Windows: {
#if defined(USE_WINDOWS)
                const SDL_PropertiesID props = SDL_GetWindowProperties(window);
                const HWND hwnd = (HWND)SDL_GetPointerProperty(
                    props, SDL_PROP_WINDOW_WIN32_HWND_POINTER, nullptr);
                const HINSTANCE hinstance = (HINSTANCE)SDL_GetPointerProperty(
                    props, SDL_PROP_WINDOW_WIN32_INSTANCE_POINTER, nullptr);
                if (hwnd == nullptr || hinstance == nullptr) {
                    return {};
                }

                const auto handle = platform::NativeWindowsHandle{
                    .hwnd = hwnd,
                    .hinstance = hinstance,
                };
#else
                const auto handle = platform::NativeWindowsHandle{};
#endif

                return platform::NativeWindowHandle{
                    .windows = handle,
                };
            }
            case platform::WindowSystem::X11: {
#if defined(USE_X11)
                const auto handle = platform::NativeX11Handle{};
#else
                const auto handle = platform::NativeX11Handle{};
#endif

                return platform::NativeWindowHandle{
                    .x11 = handle,
                };
            }
            default:
                break;
        }

        return {};
    }

    foundation::Option<platform::WindowHandle> SDL3Window::get_handle(void) const noexcept {
        const auto opt_window_handle =
            SDL3Window::get_native_handle(this->window.get(), this->window_system);
        if (opt_window_handle.is_none()) {
            return {};
        }

        const platform::WindowHandle handle{
            .id = types::HandleId{},
            .tag = this->window_system,
            .native_handle = opt_window_handle.unwrap(),
        };

        return handle;
    }

    foundation::Option<types::WindowPosition> SDL3Window::get_position(void) const noexcept {
        types::WindowPosition position{};
        if (!SDL_GetWindowPosition(this->window.get(), &position.x, &position.y)) {
            return {};
        }

        return position;
    }

    foundation::VoidResult<platform::WindowError> SDL3Window::set_position(
        const types::WindowPosition& position) noexcept {
        if (!SDL_SetWindowPosition(this->window.get(), position.x, position.y)) {
            return foundation::Error(platform::WindowError::FailedSetWinodwPosition);
        }

        return {};
    }

    foundation::Option<types::WindowSize> SDL3Window::get_size(void) const noexcept {
        types::WindowSize size{};
        if (!SDL_GetWindowSize(this->window.get(), &size.width, &size.height)) {
            return {};
        }

        return size;
    }

    foundation::VoidResult<platform::WindowError> SDL3Window::set_size(
        const types::WindowSize& size) noexcept {
        if (!SDL_SetWindowSize(this->window.get(), size.width, size.height)) {
            return foundation::Error(platform::WindowError::FailedSetWinodwSize);
        }

        return {};
    }

    foundation::VoidResult<platform::WindowError> SDL3Window::set_title(
        const std::string& title) noexcept {
        return {};
    }

    foundation::Option<std::string> SDL3Window::get_title(void) const noexcept {
        return foundation::Option<std::string>();
    }

    void SDL3Window::close(void) {
        this->is_closing = true;
    }

    bool SDL3Window::should_close(void) const {
        return !this->is_closing;
    }

    void SDL3Window::poll_events(void) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            /*
            if (ImGui_ImplSDL3_ProcessEvent(&event)) {
                continue;
            };
            */

            switch (event.type) {
                case SDL_EventType::SDL_EVENT_QUIT:
                    this->is_closing = true;
                    break;
                case SDL_EventType::SDL_EVENT_KEY_DOWN:
                    this->input.on_key_down(SDL3Input::convert_key_code(event.key.key));
                    break;
                case SDL_EventType::SDL_EVENT_KEY_UP:
                    this->input.on_key_up(SDL3Input::convert_key_code(event.key.key));
                    break;
                case SDL_EventType::SDL_EVENT_MOUSE_BUTTON_DOWN:
                    this->input.on_mouse_button_down(
                        SDL3Input::convert_mouse_button(event.button.button));
                    break;
                case SDL_EventType::SDL_EVENT_MOUSE_BUTTON_UP:
                    this->input.on_mouse_button_up(
                        SDL3Input::convert_mouse_button(event.button.button));
                    break;
                case SDL_EventType::SDL_EVENT_MOUSE_MOTION:
                    this->input.on_mouse_move(types::to_screen(
                        types::ClientMousePosition{
                            .x = static_cast<std::int32_t>(event.motion.x),
                            .y = static_cast<std::int32_t>(event.motion.y),
                        },
                        this->position));
                    break;
                case SDL_EventType::SDL_EVENT_MOUSE_WHEEL:

                    break;
                case SDL_EventType::SDL_EVENT_WINDOW_FOCUS_GAINED:
                    // SDL_StartTextInput();
                    break;
                default:
                    break;
            }
        }

        SDL_Keymod mod = SDL_GetModState();
        /*
        io.AddKeyEvent(ImGuiKey::ImGuiKey_LeftCtrl, (mod & SDL_KMOD_CTRL) != 0);
        io.AddKeyEvent(ImGuiKey::ImGuiKey_LeftShift, (mod & SDL_KMOD_SHIFT) != 0);
        io.AddKeyEvent(ImGuiKey::ImGuiMod_Alt, (mod & SDL_KMOD_ALT) != 0);
        */
    }
} // namespace enishi::platform_impl