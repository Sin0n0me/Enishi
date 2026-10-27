#include "crack_surface.h"
#include "crack_pass.h"
#include <algorithm>
#include <debug/log/logger.h>
#include <engine_types/renderer/overlay_properties.h>
#include <platform_impl/window/sdl/sdl3_window.h>
#include <string_view>
#if defined(_WIN32)
#include "crack_overlay_windows.h"
#else
#include "crack_overlay_linux.h"
#endif
namespace enishi::platform_impl {
    namespace {
        class ContextRestore {
          private:
            SDL_Window* window = SDL_GL_GetCurrentWindow();
            SDL_GLContext context;

          public:
            inline explicit ContextRestore(bool enabled = true)
                : context(enabled ? SDL_GL_GetCurrentContext() : nullptr) {
            }
            inline ~ContextRestore() {
                if (this->context != nullptr && !SDL_GL_MakeCurrent(this->window, this->context)) {
                    debug::Logger::warning(
                        std::string{"Failed to restore OpenGL context: "} + SDL_GetError());
                }
            }
        };
        struct Overlay {
            std::shared_ptr<SDL3Window> window;
            SDL_GLContext context = nullptr; // Borrowed; the renderer owns its context.
            std::unique_ptr<CrackPass> pass;
        };
        class CrackSurface final : public effect::ICrackSurface {
          private:
            std::shared_ptr<platform::IShaderDataProvider> shaders;
            std::vector<Overlay> overlays;

          public:
            explicit CrackSurface(std::shared_ptr<platform::IShaderDataProvider> shaders);
            ~CrackSurface() noexcept override;
            foundation::Result<void, effect::EffectError> open(
                const effect::CrackSettings& settings, types::TextureData mask) override;
            foundation::Result<void, effect::EffectError> draw(float opacity) override;
            void close() noexcept override;

          private:
            foundation::Result<void, effect::EffectError> add_display(SDL_DisplayID display,
                const effect::CrackSettings& settings,
                std::shared_ptr<types::TextureData> mask);
        };
        CrackSurface::CrackSurface(std::shared_ptr<platform::IShaderDataProvider> shaders)
            : shaders(std::move(shaders)) {
        }
        CrackSurface::~CrackSurface() noexcept {
            this->close();
        }
        foundation::Result<void, effect::EffectError> CrackSurface::open(
            const effect::CrackSettings& settings, types::TextureData mask) {
            if (this->shaders == nullptr || !this->overlays.empty()) {
                return foundation::Error(effect::EffectError::StartFailed,
                    "Crack surface is unavailable or already open");
            }
            const char* driver = SDL_GetCurrentVideoDriver();
            if (driver == nullptr ||
                (std::string_view{driver} != "windows" && std::string_view{driver} != "x11")) {
                return foundation::Error(effect::EffectError::StartFailed,
                    "Desktop crack overlays require Windows or X11; Wayland global positioning is "
                    "unavailable");
            }
            ContextRestore restore;
            int count = 0;
            std::unique_ptr<SDL_DisplayID, decltype(&SDL_free)> displays{
                SDL_GetDisplays(&count), SDL_free};
            if (displays == nullptr || count < 1) {
                return foundation::Error(effect::EffectError::StartFailed,
                    std::string{"No displays available for crack overlay: "} + SDL_GetError());
            }
            auto shared_mask = std::make_shared<types::TextureData>(std::move(mask));
            for (int index = 0; index < count; ++index) {
                auto added = this->add_display(displays.get()[index], settings, shared_mask);
                if (added.is_err()) {
                    this->close();
                    return added;
                }
            }
            return {};
        }
        foundation::Result<void, effect::EffectError> CrackSurface::add_display(
            SDL_DisplayID display,
            const effect::CrackSettings& settings,
            std::shared_ptr<types::TextureData> mask) {
            SDL_Rect bounds{};
            if (!SDL_GetDisplayBounds(display, &bounds)) {
                return foundation::Error(
                    effect::EffectError::StartFailed, std::string{SDL_GetError()});
            }
            const auto backend = render_backend_description();
            const auto system = std::string_view{SDL_GetCurrentVideoDriver()} == "windows"
                                    ? platform::WindowSystem::Windows
                                    : platform::WindowSystem::X11;
            auto& overlay = this->overlays.emplace_back();
            overlay.window = std::make_shared<SDL3Window>("Enishi Crack",
                types::WindowSize{bounds.w, bounds.h},
                system,
                backend.api,
                SDL_WINDOW_HIDDEN | SDL_WINDOW_NOT_FOCUSABLE | SDL_WINDOW_UTILITY);
            auto initialized = overlay.window->init();
            if (initialized.is_err()) {
                return initialized.propagation(effect::EffectError::StartFailed);
            }
            auto* native = overlay.window->get_window_handle().unwrap();
            if (!SDL_SetWindowPosition(native, bounds.x, bounds.y) ||
                !prepare_crack_overlay(native)) {
                return foundation::Error(
                    effect::EffectError::StartFailed, std::string{SDL_GetError()});
            }
            types::WindowSize pixels{};
            if (!SDL_GetWindowSizeInPixels(native, &pixels.width, &pixels.height)) {
                return foundation::Error(
                    effect::EffectError::StartFailed, std::string{SDL_GetError()});
            }
            if (!SDL_SetBooleanProperty(
                    SDL_GetWindowProperties(native), types::REQUIRE_TRANSPARENT_SURFACE, true)) {
                return foundation::Error(
                    effect::EffectError::StartFailed, std::string{SDL_GetError()});
            }
            auto graphics = create_renderer(overlay.window, pixels);
            if (graphics.is_err()) {
                return graphics.propagation(effect::EffectError::StartFailed);
            }
            if (backend.api == types::GraphicsAPI::OpenGL40) {
                overlay.context = SDL_GL_GetCurrentContext();
            }
            auto viewport = graphics.unwrap().renderer->create_viewport(
                {0, 0, static_cast<float>(pixels.width), static_cast<float>(pixels.height), 0, 1});
            if (viewport.is_err()) {
                return viewport.propagation(effect::EffectError::StartFailed);
            }
            overlay.pass = std::make_unique<CrackPass>(std::move(graphics.unwrap_mut()), settings);
            auto built = overlay.pass->initialize(*overlay.window, *this->shaders, std::move(mask));
            if (built.is_err()) {
                return built.propagation(effect::EffectError::StartFailed);
            }
            return {};
        }
        foundation::Result<void, effect::EffectError> CrackSurface::draw(float opacity) {
            ContextRestore restore;
            for (auto& overlay : this->overlays) {
                auto* window = overlay.window->get_window_handle().unwrap();
                if (overlay.context != nullptr && !SDL_GL_MakeCurrent(window, overlay.context)) {
                    return foundation::Error(
                        effect::EffectError::UpdateFailed, std::string{SDL_GetError()});
                }
                auto drawn = overlay.pass->draw(opacity);
                if (drawn.is_err()) {
                    return drawn.propagation(effect::EffectError::UpdateFailed);
                }
                if ((SDL_GetWindowFlags(window) & SDL_WINDOW_HIDDEN) != 0 &&
                    !SDL_ShowWindow(window)) {
                    return foundation::Error(
                        effect::EffectError::UpdateFailed, std::string{SDL_GetError()});
                }
            }
            return {};
        }
        void CrackSurface::close() noexcept {
            if (this->overlays.empty()) {
                return;
            }
            const auto* current = SDL_GL_GetCurrentWindow();
            const bool owns_current = std::any_of(
                this->overlays.begin(), this->overlays.end(), [current](const auto& overlay) {
                    return overlay.window != nullptr &&
                           overlay.window->get_window_handle().is_some() &&
                           overlay.window->get_window_handle().unwrap() == current;
                });
            ContextRestore restore{!owns_current};
            // Destroy each renderer while its window and GL context still exist.
            for (auto& overlay : this->overlays) {
                overlay.pass.reset();
            }
            this->overlays.clear();
        }
    } // namespace
    std::unique_ptr<effect::ICrackSurface> create_crack_surface(
        std::shared_ptr<platform::IShaderDataProvider> shaders) {
        return std::make_unique<CrackSurface>(std::move(shaders));
    }
} // namespace enishi::platform_impl
