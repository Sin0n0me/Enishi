#include "effect_surface.h"
#include "effect_display_target.h"
#include <algorithm>
#include <debug/log/logger.h>
#include <engine_types/renderer/overlay_properties.h>
#include <platform_impl/renderer/renderer_factory.h>
#include <platform_impl/window/sdl/sdl3_window.h>
#include <platform_impl/window/sdl/sdl_gl_context_restore.h>
#include <string_view>
#if defined(_WIN32)
#include "effect_overlay_windows.h"
#else
#include "effect_overlay_linux.h"
#endif
namespace enishi::platform_impl {
    struct EffectSurface::Overlay {
        std::shared_ptr<SDL3Window> window;
        SDL_Rect bounds{};
        SDL_DisplayID display = 0;
        SDL_GLContext context = nullptr; // Borrowed; graphics owns the context.
        RendererInstance graphics;
        std::unique_ptr<IEffectSurfaceContent> content;
    };
    EffectSurface::EffectSurface() = default;
    EffectSurface::~EffectSurface() noexcept {
        this->close();
    }
    foundation::Result<void, effect::EffectError> EffectSurface::open(
        const ContentFactory& factory) {
        if (factory == nullptr || !this->overlays.empty()) {
            return foundation::Error(
                effect::EffectError::StartFailed, "Effect surface is unavailable or already open");
        }
        const char* driver = SDL_GetCurrentVideoDriver();
        if (driver == nullptr ||
            (std::string_view{driver} != "windows" && std::string_view{driver} != "x11")) {
            return foundation::Error(effect::EffectError::StartFailed,
                "Desktop effect overlays require Windows or X11; Wayland global positioning is "
                "unavailable");
        }
        SDLGLContextRestore restore;
        int count = 0;
        std::unique_ptr<SDL_DisplayID, decltype(&SDL_free)> displays{
            SDL_GetDisplays(&count), SDL_free};
        if (displays == nullptr || count < 1) {
            return foundation::Error(effect::EffectError::StartFailed,
                std::string{"No displays available for effect overlay: "} + SDL_GetError());
        }
        for (int index = 0; index < count; ++index) {
            auto added = this->add_display(displays.get()[index], factory);
            if (added.is_err()) {
                this->close();
                return added;
            }
        }
        double left = this->overlays.front().bounds.x;
        double top = this->overlays.front().bounds.y;
        double right = left;
        double bottom = top;
        for (const auto& overlay : this->overlays) {
            left = (std::min)(left, static_cast<double>(overlay.bounds.x));
            top = (std::min)(top, static_cast<double>(overlay.bounds.y));
            right = (std::max)(right, static_cast<double>(overlay.bounds.x) + overlay.bounds.w);
            bottom = (std::max)(bottom, static_cast<double>(overlay.bounds.y) + overlay.bounds.h);
        }
        // One shared scale keeps the pattern continuous across display boundaries.
        this->desktop_span = static_cast<float>((std::max)(right - left, bottom - top));
        return {};
    }
    foundation::Result<void, effect::EffectError> EffectSurface::add_display(
        std::uint32_t display, const ContentFactory& factory) {
        SDL_Rect bounds{};
        if (!SDL_GetDisplayBounds(display, &bounds)) {
            return foundation::Error(effect::EffectError::StartFailed, std::string{SDL_GetError()});
        }
        const auto backend = render_backend_description();
        const auto system = std::string_view{SDL_GetCurrentVideoDriver()} == "windows"
                                ? platform::WindowSystem::Windows
                                : platform::WindowSystem::X11;
        auto& overlay = this->overlays.emplace_back();
        overlay.bounds = bounds;
        overlay.display = display;
        overlay.window = std::make_shared<SDL3Window>("Enishi Effect",
            types::WindowSize{bounds.w, bounds.h},
            system,
            backend.api,
            SDL_WINDOW_HIDDEN | SDL_WINDOW_NOT_FOCUSABLE | SDL_WINDOW_UTILITY);
        auto initialized = overlay.window->init();
        if (initialized.is_err()) {
            return initialized.propagation(effect::EffectError::StartFailed);
        }
        auto* native = overlay.window->get_window_handle().unwrap();
        if (!SDL_SetWindowPosition(native, bounds.x, bounds.y) || !prepare_effect_overlay(native)) {
            return foundation::Error(effect::EffectError::StartFailed, std::string{SDL_GetError()});
        }
        types::WindowSize pixels{};
        if (!SDL_GetWindowSizeInPixels(native, &pixels.width, &pixels.height)) {
            return foundation::Error(effect::EffectError::StartFailed, std::string{SDL_GetError()});
        }
        if (!SDL_SetBooleanProperty(
                SDL_GetWindowProperties(native), types::REQUIRE_TRANSPARENT_SURFACE, true)) {
            return foundation::Error(effect::EffectError::StartFailed, std::string{SDL_GetError()});
        }
        auto graphics = create_renderer(overlay.window, pixels);
        if (graphics.is_err()) {
            return graphics.propagation(effect::EffectError::StartFailed);
        }
        overlay.graphics = std::move(graphics.unwrap_mut());
        if (backend.api == types::GraphicsAPI::OpenGL40) {
            overlay.context = SDL_GL_GetCurrentContext();
        }
        auto viewport = overlay.graphics.renderer->create_viewport(
            {0, 0, static_cast<float>(pixels.width), static_cast<float>(pixels.height), 0, 1});
        if (viewport.is_err()) {
            return viewport.propagation(effect::EffectError::StartFailed);
        }
        overlay.content = factory();
        if (overlay.content == nullptr) {
            return foundation::Error(
                effect::EffectError::InvalidArgument, "Effect surface content is missing");
        }
        auto initialized_content = overlay.content->initialize(
            {overlay.graphics.renderer, overlay.graphics.encoder, *overlay.window, backend});
        if (initialized_content.is_err()) {
            return initialized_content.propagation(effect::EffectError::StartFailed);
        }
        return {};
    }
    foundation::Result<void, effect::EffectError> EffectSurface::draw(
        const effect::EffectTarget& target) {
        if (this->overlays.empty()) {
            return foundation::Error(
                effect::EffectError::UpdateFailed, "Effect surface is not open");
        }
        if (target.layer != effect::EffectLayer::Foreground &&
            target.layer != effect::EffectLayer::Background) {
            return foundation::Error(effect::EffectError::InvalidArgument, "Unknown effect layer");
        }
        auto resolved = resolve_effect_display(target);
        if (resolved.is_err()) {
            return std::move(resolved).take_err();
        }
        const auto selected = resolved.unwrap();
        if (selected.has_value() &&
            std::none_of(this->overlays.begin(),
                this->overlays.end(),
                [&selected](const auto& overlay) { return overlay.display == *selected; })) {
            return foundation::Error(effect::EffectError::UpdateFailed,
                "Target display is unavailable; prepare the effect again after display "
                "changes");
        }
        SDLGLContextRestore restore;
        for (auto& overlay : this->overlays) {
            auto* window = overlay.window->get_window_handle().unwrap();
            if (selected.has_value() && overlay.display != *selected) {
                if ((SDL_GetWindowFlags(window) & SDL_WINDOW_HIDDEN) == 0 &&
                    !SDL_HideWindow(window)) {
                    return foundation::Error(
                        effect::EffectError::UpdateFailed, std::string{SDL_GetError()});
                }
                continue;
            }
            if (overlay.context != nullptr && !SDL_GL_MakeCurrent(window, overlay.context)) {
                return foundation::Error(
                    effect::EffectError::UpdateFailed, std::string{SDL_GetError()});
            }
            const auto& bounds = overlay.bounds;
            auto drawn = overlay.content->draw({bounds.x,
                bounds.y,
                bounds.w,
                bounds.h,
                this->desktop_span,
                !selected.has_value()});
            if (drawn.is_err()) {
                return drawn.propagation(effect::EffectError::UpdateFailed);
            }
            if ((SDL_GetWindowFlags(window) & SDL_WINDOW_HIDDEN) != 0) {
                if (!set_effect_overlay_layer(window, target.layer) || !SDL_ShowWindow(window)) {
                    return foundation::Error(
                        effect::EffectError::UpdateFailed, std::string{SDL_GetError()});
                }
            }
            if (!set_effect_overlay_layer(window, target.layer)) {
                return foundation::Error(
                    effect::EffectError::UpdateFailed, std::string{SDL_GetError()});
            }
        }
        return {};
    }
    bool EffectSurface::hide() noexcept {
        bool hidden = true;
        for (auto& overlay : this->overlays) {
            auto* window = overlay.window->get_window_handle().unwrap();
            if (!SDL_HideWindow(window)) {
                debug::Logger::warning(
                    std::string{"Failed to hide effect overlay: "} + SDL_GetError());
                // A failed display must not prevent the remaining overlays from being hidden.
                hidden = false;
            }
        }
        return hidden;
    }
    void EffectSurface::close() noexcept {
        if (this->overlays.empty()) {
            return;
        }
        const auto* current = SDL_GL_GetCurrentWindow();
        const bool owns_current = std::any_of(
            this->overlays.begin(), this->overlays.end(), [current](const auto& overlay) {
                return overlay.window != nullptr && overlay.window->get_window_handle().is_some() &&
                       overlay.window->get_window_handle().unwrap() == current;
            });
        SDLGLContextRestore restore{!owns_current};
        // Destroy each renderer while its window and GL context still exist.
        for (auto& overlay : this->overlays) {
            if (overlay.context != nullptr &&
                !SDL_GL_MakeCurrent(
                    overlay.window->get_window_handle().unwrap(), overlay.context)) {
                debug::Logger::warning(
                    std::string{"Failed to activate effect context for cleanup: "} +
                    SDL_GetError());
            }
            overlay.content.reset();
            overlay.graphics.encoder.reset();
            overlay.graphics.renderer.reset();
        }
        this->overlays.clear();
    }
} // namespace enishi::platform_impl
