#include "crack_render_tests.h"
#include "../../../core/tests/test_runner.h"
#include <SDL3/SDL.h>
#include <effect/crack/crack_effect.h>
#include <effect/effect_manager.h>
#include <fstream>
#include <iostream>
#include <iterator>
#include <limits>
#include <type_traits>
#if defined(_WIN32)
#include <Windows.h>
#endif
#include <platform_impl/effect/crack_surface.h>
#include <platform_impl/renderer/renderer_factory.h>
#include <platform_impl/window/sdl/sdl_gl_context_restore.h>
namespace enishi::tests {
    namespace {
        class Shaders final : public platform::IShaderDataProvider {
          public:
            bool force_failure = false;
            mutable unsigned loads = 0;

          public:
            foundation::Result<std::vector<platform::ShaderDataEntry>, platform::RenderError> get(
                std::span<const std::tuple<types::ShaderKind, std::filesystem::path>> paths)
                const override;
        };
        foundation::Result<std::vector<platform::ShaderDataEntry>, platform::RenderError>
        Shaders::get(
            std::span<const std::tuple<types::ShaderKind, std::filesystem::path>> paths) const {
            ++this->loads;
            if (this->force_failure) {
                return foundation::Error(
                    platform::RenderError::MakeError, "Injected shader load failure");
            }
            const auto api = platform_impl::render_backend_description().api;
            auto binary_type = types::ShaderBinaryType::DXBC;
            std::string extension = ".dxbc";
            if (api == types::GraphicsAPI::OpenGL40) {
                binary_type = types::ShaderBinaryType::SourceFileGLSL;
                extension = ".glsl";
            }
            if (api == types::GraphicsAPI::Vulkan11) {
                binary_type = types::ShaderBinaryType::SPIR_V;
                extension = ".spv";
            }
            std::vector<platform::ShaderDataEntry> entries;
            for (const auto& [kind, name] : paths) {
                const auto path = platform_impl::shader_directory() / (name.string() + extension);
                std::ifstream file{path, std::ios::binary};
                if (!file.is_open()) {
                    return foundation::Error(
                        platform::RenderError::MakeError, "Missing shader: " + path.string());
                }
                auto data = std::make_shared<types::ShaderData>();
                data->binary_type = binary_type;
                data->code = {
                    std::istreambuf_iterator<char>{file}, std::istreambuf_iterator<char>{}};
                entries.push_back({kind, path, std::move(data)});
            }
            return entries;
        }
        int window_count() {
            int count = 0;
            std::unique_ptr<SDL_Window*, decltype(&SDL_free)> windows{
                SDL_GetWindows(&count), SDL_free};
            check(windows != nullptr, SDL_GetError());
            return count;
        }
        void input_transparency() {
#if defined(_WIN32)
            int count = 0;
            std::unique_ptr<SDL_Window*, decltype(&SDL_free)> windows{
                SDL_GetWindows(&count), SDL_free};
            check(windows != nullptr, SDL_GetError());
            for (int index = 0; index < count; ++index) {
                const auto native = static_cast<HWND>(
                    SDL_GetPointerProperty(SDL_GetWindowProperties(windows.get()[index]),
                        SDL_PROP_WINDOW_WIN32_HWND_POINTER,
                        nullptr));
                RECT bounds{};
                check(native != nullptr && GetWindowRect(native, &bounds) != 0,
                    "overlay has native bounds");
                const POINT center{bounds.left + (bounds.right - bounds.left) / 2,
                    bounds.top + (bounds.bottom - bounds.top) / 2};
                check(WindowFromPoint(center) != native,
                    "overlay must not intercept desktop hit testing");
            }
#endif
        }
        void context_restoration() {
            check(SDL_Init(SDL_INIT_VIDEO), SDL_GetError());
            constexpr int HIDDEN_WINDOW_SIZE = 16;
            {
                std::unique_ptr<SDL_Window, decltype(&SDL_DestroyWindow)> window{
                    SDL_CreateWindow("Context restoration",
                        HIDDEN_WINDOW_SIZE,
                        HIDDEN_WINDOW_SIZE,
                        SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN),
                    SDL_DestroyWindow};
                check(window != nullptr, SDL_GetError());
                std::unique_ptr<std::remove_pointer_t<SDL_GLContext>,
                    decltype(&SDL_GL_DestroyContext)>
                    context{nullptr, SDL_GL_DestroyContext};
                check(SDL_GL_GetCurrentContext() == nullptr, "fixture begins without a GL context");
                {
                    platform_impl::SDLGLContextRestore restore;
                    context.reset(SDL_GL_CreateContext(window.get()));
                    check(context != nullptr, SDL_GetError());
                }
                check(SDL_GL_GetCurrentContext() == nullptr && SDL_GL_GetCurrentWindow() == nullptr,
                    "guard detaches a context when the caller had none");
                check(SDL_GL_MakeCurrent(window.get(), context.get()), SDL_GetError());
                {
                    platform_impl::SDLGLContextRestore restore;
                    check(SDL_GL_MakeCurrent(nullptr, nullptr), SDL_GetError());
                }
                check(SDL_GL_GetCurrentContext() == context.get() &&
                          SDL_GL_GetCurrentWindow() == window.get(),
                    "guard restores the caller's context and window");
                check(SDL_GL_MakeCurrent(nullptr, nullptr), SDL_GetError());
            }
            SDL_Quit();
        }
        void surface_contract() {
            auto surface = platform_impl::create_crack_surface(std::make_shared<Shaders>());
            check(surface->draw(1).is_err(), "unopened surface cannot draw");
            effect::CrackSettings settings;
            settings.mask_size = 64;
            auto mask = effect::make_crack_mask(settings);
            check(mask.is_ok(), "mask fixture is valid");
            mask.unwrap_mut().mips.front().pixels.pop_back();
            auto invalid = surface->open(settings, std::move(mask.unwrap_mut()));
            check(invalid.is_err() &&
                      invalid.unwrap_err().get_error() == effect::EffectError::InvalidArgument,
                "truncated mask rejected before creating renderer resources");
            surface->close();
            surface->close();
        }
        void prepared_overlays() {
            check(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS), SDL_GetError());
            const int original = window_count();
            auto shaders = std::make_shared<Shaders>();
            using Clock = std::chrono::steady_clock;
            const auto milliseconds = [](Clock::duration duration) {
                return std::chrono::duration<double, std::milli>{duration}.count();
            };
            {
                effect::CrackEffect cold{{}, platform_impl::create_crack_surface(shaders)};
                const auto begin = Clock::now();
                check(cold.start().is_ok(), "cold reference starts");
                std::cout << "Cold crack start (ms): " << milliseconds(Clock::now() - begin)
                          << '\n';
                cold.stop();
            }
            {
                auto crack = std::make_shared<effect::CrackEffect>(
                    effect::CrackSettings{}, platform_impl::create_crack_surface(shaders));
                effect::EffectManager manager;
                check(crack->prepare().is_ok(), "prepare real default-size crack");
                const auto loads = shaders->loads;
                const int prepared_windows = window_count();
                check(prepared_windows > original, "prepared resources exist before click");
                for (int repeat = 0; repeat < 3; ++repeat) {
                    const auto begin = Clock::now();
                    crack->set_impact(effect::CrackImpact{repeat * 200, 150});
                    auto id = manager.play_shared(crack);
                    check(id.is_ok(), "prepared crack starts");
                    std::cout << "Prepared crack start (ms): " << milliseconds(Clock::now() - begin)
                              << '\n';
                    check(manager.stop(id.unwrap()).is_ok(), "prepared crack stops");
                    check(window_count() == prepared_windows && shaders->loads == loads,
                        "replay neither recreates windows nor reloads shaders");
                    int count = 0;
                    std::unique_ptr<SDL_Window*, decltype(&SDL_free)> windows{
                        SDL_GetWindows(&count), SDL_free};
                    check(windows != nullptr, SDL_GetError());
                    for (int index = 0; index < count; ++index) {
                        check((SDL_GetWindowFlags(windows.get()[index]) & SDL_WINDOW_HIDDEN) != 0,
                            "stopped prepared overlay stays hidden");
                    }
                }
            }
            check(window_count() == original, "prepared resources released with owner");
            SDL_Quit();
        }
        void check_visible_displays(SDL_DisplayID selected, int expected) {
            int count = 0;
            std::unique_ptr<SDL_Window*, decltype(&SDL_free)> windows{
                SDL_GetWindows(&count), SDL_free};
            check(windows != nullptr, SDL_GetError());
            int visible = 0;
            for (int index = 0; index < count; ++index) {
                auto* window = windows.get()[index];
                if ((SDL_GetWindowFlags(window) & SDL_WINDOW_HIDDEN) == 0) {
                    ++visible;
                    check(selected == 0 || SDL_GetDisplayForWindow(window) == selected,
                        "visible overlay belongs only to the selected display");
                }
            }
            check(visible == expected, "visible overlay count matches the display target");
        }
        void targeted_overlays() {
            check(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS), SDL_GetError());
            int display_count = 0;
            std::unique_ptr<SDL_DisplayID, decltype(&SDL_free)> displays{
                SDL_GetDisplays(&display_count), SDL_free};
            check(displays != nullptr && display_count > 0, SDL_GetError());
            auto shaders = std::make_shared<Shaders>();
            {
                effect::CrackSettings settings;
                settings.mask_size = 64;
                auto mask = effect::make_crack_mask(settings);
                check(mask.is_ok(), "target mask generated");
                auto surface = platform_impl::create_crack_surface(shaders);
                check(surface->open(settings, std::move(mask.unwrap_mut())).is_ok(),
                    "target resources prepared");
                const auto loads = shaders->loads;
                check(surface->draw(1).is_ok(), "all-display target draws");
                check_visible_displays(0, display_count);
                for (int index = 0; index < display_count; ++index) {
                    SDL_Rect bounds{};
                    check(SDL_GetDisplayBounds(displays.get()[index], &bounds), SDL_GetError());
                    const effect::CrackImpact center{
                        bounds.x + bounds.w / 2, bounds.y + bounds.h / 2};
                    check(
                        surface
                            ->draw(1, center, effect::EffectTarget::display_at(center.x, center.y))
                            .is_ok(),
                        "target changes during playback without rebuilding");
                    check_visible_displays(displays.get()[index], 1);
                }
                check(surface->draw(1, std::nullopt, effect::EffectTarget::all_displays()).is_ok(),
                    "all displays can be enabled again");
                check_visible_displays(0, display_count);
                effect::EffectTarget invalid;
                invalid.kind = static_cast<effect::EffectTargetKind>(-1);
                check(surface->draw(1, std::nullopt, invalid).is_err(), "unknown target rejected");
                check(shaders->loads == loads && window_count() == display_count,
                    "target changes reuse the prepared shaders and windows");
                check(surface->hide(), "overlay hides successfully");
                check_visible_displays(0, 0);
            }
            check(window_count() == 0, "targeted windows released");
            std::cout << "Validated display targeting on " << display_count << " displays\n";
            SDL_Quit();
        }
        void overlays() {
            check(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS), SDL_GetError());
            const int original = window_count();
            auto shaders = std::make_shared<Shaders>();
            effect::CrackSettings settings;
            settings.mask_size = 64;
            for (int repeat = 0; repeat < 2; ++repeat) {
                effect::CrackEffect first{settings, platform_impl::create_crack_surface(shaders)};
                effect::CrackEffect second{settings, platform_impl::create_crack_surface(shaders)};
                for (auto* effect : {&first, &second}) {
                    const auto previous_context = SDL_GL_GetCurrentContext();
                    auto* previous_window = SDL_GL_GetCurrentWindow();
                    auto started = effect->start();
                    if (started.is_err()) {
                        check(false, started.unwrap_err().get_message().c_str());
                    }
                    check(SDL_GL_GetCurrentContext() == previous_context &&
                              SDL_GL_GetCurrentWindow() == previous_window,
                        "start restores the previous GL context, including no context");
                    auto updated = effect->update(types::DeltaTime{std::chrono::milliseconds{16}});
                    if (updated.is_err()) {
                        check(false, updated.unwrap_err().get_message().c_str());
                    }
                    check(SDL_GL_GetCurrentContext() == previous_context &&
                              SDL_GL_GetCurrentWindow() == previous_window,
                        "draw restores the previous GL context");
                }
                check(window_count() > original, "effects own display overlays");
                input_transparency();
                first.stop();
                second.stop();
                check(window_count() == original, "all overlay windows released");
                check((SDL_WasInit(SDL_INIT_VIDEO) & SDL_INIT_VIDEO) != 0,
                    "effects preserve application SDL initialization");
            }
            auto surface = platform_impl::create_crack_surface(shaders);
            auto mask = effect::make_crack_mask(settings);
            check(mask.is_ok(), "opacity test mask is valid");
            check(surface->open(settings, std::move(mask.unwrap_mut())).is_ok(),
                "opacity test surface opens");
            for (const float opacity : {-1.0f, 2.0f, std::numeric_limits<float>::quiet_NaN()}) {
                auto drawn = surface->draw(opacity);
                check(drawn.is_err() &&
                          drawn.unwrap_err().get_error() == effect::EffectError::InvalidArgument,
                    "invalid opacity rejected before GPU upload");
            }
            surface->close();
            shaders->force_failure = true;
            effect::CrackEffect failed{settings, platform_impl::create_crack_surface(shaders)};
            check(failed.start().is_err(), "shader failure must propagate from effect start");
            check(
                window_count() == original, "partial renderer initialization releases its windows");
            check((SDL_WasInit(SDL_INIT_VIDEO) & SDL_INIT_VIDEO) != 0,
                "failed initialization preserves the application SDL reference");
            SDL_Quit();
        }
    } // namespace
} // namespace enishi::tests
int main() {
    enishi::tests::run_test("SDL GL context restoration", enishi::tests::context_restoration);
    enishi::tests::run_test("Crack surface input contract", enishi::tests::surface_contract);
    enishi::tests::run_test("Prepared crack renderer reuse", enishi::tests::prepared_overlays);
    enishi::tests::run_test("Crack display targeting", enishi::tests::targeted_overlays);
    enishi::tests::run_test("Crack overlay renderer lifecycle", enishi::tests::overlays);
}
