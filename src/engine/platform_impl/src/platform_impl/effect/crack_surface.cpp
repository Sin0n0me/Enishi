#include "crack_surface.h"
#include "effect_display_target.h"
#include <algorithm>
#include <cmath>
#include <debug/log/logger.h>
#include <engine_types/renderer/overlay_properties.h>
#include <platform_impl/renderer/renderer_factory.h>
#include <platform_impl/window/sdl/sdl3_window.h>
#include <platform_impl/window/sdl/sdl_gl_context_restore.h>
#include <render_pass/crack_pass.h>
#include <string_view>
#if defined(_WIN32)
#include "crack_overlay_windows.h"
#else
#include "crack_overlay_linux.h"
#endif
namespace enishi::platform_impl {
    namespace {
        foundation::Result<void, effect::EffectError> validate_surface_input(
            const effect::CrackSettings& settings, const types::TextureData& mask) {
            auto valid = effect::validate_crack_settings(settings);
            if (valid.is_err()) {
                return valid;
            }
            constexpr std::uint32_t RGBA_CHANNELS = 4;
            // A tightly packed mask requires row_bytes = width * channels and total = row_bytes *
            // height.
            const auto row_bytes = settings.mask_size * RGBA_CHANNELS;
            const auto total_bytes = row_bytes * settings.mask_size;
            if (mask.format != types::TextureFormat::RGBA8_UNORM || mask.is_cubemap ||
                mask.width != settings.mask_size || mask.height != settings.mask_size ||
                mask.depth != 1 || mask.array_size != 1 || mask.mips.size() != 1) {
                return foundation::Error(
                    effect::EffectError::InvalidArgument, "Invalid crack surface mask layout");
            }
            const auto& mip = mask.mips.front();
            if (mip.width != mask.width || mip.height != mask.height ||
                mip.row_pitch != row_bytes || mip.slice_pitch != total_bytes ||
                mip.pixels.size() != total_bytes) {
                return foundation::Error(
                    effect::EffectError::InvalidArgument, "Invalid crack surface mask storage");
            }
            return {};
        }
        struct Overlay {
            std::shared_ptr<SDL3Window> window;
            SDL_Rect bounds{};
            SDL_DisplayID display = 0;
            SDL_GLContext context = nullptr; // Borrowed; the renderer owns its context.
            std::unique_ptr<render_pass::CrackPass> pass;
        };
        class CrackSurface final : public effect::ICrackSurface {
          private:
            std::shared_ptr<platform::IShaderDataProvider> shaders;
            std::vector<Overlay> overlays;
            bool radial = false;
            float desktop_span = 1;

          public:
            explicit CrackSurface(std::shared_ptr<platform::IShaderDataProvider> shaders);
            ~CrackSurface() noexcept override;
            foundation::Result<void, effect::EffectError> open(
                const effect::CrackSettings& settings, types::TextureData mask) override;
            foundation::Result<void, effect::EffectError> draw(float opacity,
                std::optional<effect::CrackImpact> impact,
                const effect::EffectTarget& target) override;
            bool hide() noexcept override;
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
            auto valid = validate_surface_input(settings, mask);
            if (valid.is_err()) {
                return valid;
            }
            const char* driver = SDL_GetCurrentVideoDriver();
            if (driver == nullptr ||
                (std::string_view{driver} != "windows" && std::string_view{driver} != "x11")) {
                return foundation::Error(effect::EffectError::StartFailed,
                    "Desktop crack overlays require Windows or X11; Wayland global positioning is "
                    "unavailable");
            }
            SDLGLContextRestore restore;
            int count = 0;
            std::unique_ptr<SDL_DisplayID, decltype(&SDL_free)> displays{
                SDL_GetDisplays(&count), SDL_free};
            if (displays == nullptr || count < 1) {
                return foundation::Error(effect::EffectError::StartFailed,
                    std::string{"No displays available for crack overlay: "} + SDL_GetError());
            }
            this->radial = settings.pattern == effect::CrackPattern::Radial;
            auto shared_mask = std::make_shared<types::TextureData>(std::move(mask));
            for (int index = 0; index < count; ++index) {
                auto added = this->add_display(displays.get()[index], settings, shared_mask);
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
                bottom =
                    (std::max)(bottom, static_cast<double>(overlay.bounds.y) + overlay.bounds.h);
            }
            // One shared scale keeps the pattern continuous across display boundaries.
            this->desktop_span = static_cast<float>((std::max)(right - left, bottom - top));
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
            overlay.bounds = bounds;
            overlay.display = display;
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
            overlay.pass =
                std::make_unique<render_pass::CrackPass>(std::move(graphics.unwrap_mut().renderer),
                    std::move(graphics.unwrap_mut().encoder),
                    render_pass::CrackPass::Settings{.view_x = settings.view_x,
                        .view_y = settings.view_y,
                        .depth = settings.depth,
                        .mask_size = settings.mask_size,
                        .opacity = settings.opacity});
            auto built =
                overlay.pass->initialize(*overlay.window, *this->shaders, backend, std::move(mask));
            if (built.is_err()) {
                return built.propagation(effect::EffectError::StartFailed);
            }
            // Submit while hidden so first-use shader/upload work happens during preparation.
            auto warmed = overlay.pass->draw(0.0f);
            if (warmed.is_err()) {
                return warmed.propagation(effect::EffectError::StartFailed);
            }
            return {};
        }
        foundation::Result<void, effect::EffectError> CrackSurface::draw(float opacity,
            std::optional<effect::CrackImpact> impact,
            const effect::EffectTarget& target) {
            if (this->overlays.empty()) {
                return foundation::Error(
                    effect::EffectError::UpdateFailed, "Crack surface is not open");
            }
            if (!std::isfinite(opacity) || opacity < 0 || opacity > 1) {
                return foundation::Error(effect::EffectError::InvalidArgument,
                    "Crack opacity must be finite and between zero and one");
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
                glm::vec4 transform{1, 1, 0, 0};
                if (this->radial) {
                    constexpr float MASK_CENTER = 0.5f;
                    const auto& bounds = overlay.bounds;
                    const float span = impact.has_value() && !selected.has_value()
                                           ? this->desktop_span
                                           : static_cast<float>((std::max)(bounds.w, bounds.h));
                    // mask_uv = 0.5 + (desktop_pixel - impact_pixel) / (2 * desktop_span).
                    const float scale = MASK_CENTER / span;
                    const float origin_x = impact.has_value() ? static_cast<float>(impact->x)
                                                              : bounds.x + bounds.w * MASK_CENTER;
                    const float origin_y = impact.has_value() ? static_cast<float>(impact->y)
                                                              : bounds.y + bounds.h * MASK_CENTER;
                    transform = {bounds.w * scale,
                        bounds.h * scale,
                        MASK_CENTER + (bounds.x - origin_x) * scale,
                        MASK_CENTER + (bounds.y - origin_y) * scale};
                }
                auto drawn = overlay.pass->draw(opacity, transform);
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
        bool CrackSurface::hide() noexcept {
            for (auto& overlay : this->overlays) {
                auto* window = overlay.window->get_window_handle().unwrap();
                if (!SDL_HideWindow(window)) {
                    debug::Logger::warning(
                        std::string{"Failed to hide crack overlay: "} + SDL_GetError());
                    return false;
                }
            }
            return true;
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
            SDLGLContextRestore restore{!owns_current};
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
