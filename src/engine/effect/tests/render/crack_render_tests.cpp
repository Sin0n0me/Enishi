#include "crack_render_tests.h"
#include "../../../core/tests/test_runner.h"
#include <SDL3/SDL.h>
#include <algorithm>
#include <array>
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
#include <platform_impl/effect/effect_surface.h>
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
        struct ContentState {
            unsigned initialized = 0;
            unsigned draws = 0;
            unsigned destroyed = 0;
            bool fail_initialize = false;
            bool fail_draw = true;
            std::vector<platform_impl::EffectSurfaceFrame> frames;
        };
        class ProbeContent final : public platform_impl::IEffectSurfaceContent {
          private:
            std::shared_ptr<ContentState> state;

          public:
            explicit ProbeContent(std::shared_ptr<ContentState> state);
            ~ProbeContent() noexcept override;
            platform::RenderResult<void> initialize(
                const platform_impl::EffectSurfaceContext& context) override;
            platform::RenderResult<void> draw(
                const platform_impl::EffectSurfaceFrame& frame) override;
        };
        ProbeContent::ProbeContent(std::shared_ptr<ContentState> state)
            : state(std::move(state)) {
        }
        ProbeContent::~ProbeContent() noexcept {
            ++this->state->destroyed;
        }
        platform::RenderResult<void> ProbeContent::initialize(
            const platform_impl::EffectSurfaceContext& context) {
            ++this->state->initialized;
            check(context.renderer != nullptr && context.encoder != nullptr &&
                      context.window.get_size().is_some(),
                "generic content receives renderer and window interfaces");
            if (this->state->fail_initialize) {
                return foundation::Error(
                    platform::RenderError::MakeError, "Injected content initialization failure");
            }
            return {};
        }
        platform::RenderResult<void> ProbeContent::draw(
            const platform_impl::EffectSurfaceFrame& frame) {
            ++this->state->draws;
            this->state->frames.push_back(frame);
            check(frame.width > 0 && frame.height > 0 && frame.desktop_span > 0,
                "generic content receives desktop geometry");
            if (this->state->fail_draw) {
                return foundation::Error(
                    platform::RenderError::ResolveError, "Injected content draw failure");
            }
            return {};
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
        void generic_surface_lifecycle() {
            check(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS), SDL_GetError());
            const auto original = window_count();
            auto state = std::make_shared<ContentState>();
            platform_impl::EffectSurface surface;
            check(surface.open({}).is_err(), "empty content factory rejected");
            check(surface.draw(effect::EffectTarget::all_displays()).is_err(),
                "unopened generic surface cannot draw");
            check(surface.open([state]() { return std::make_unique<ProbeContent>(state); }).is_ok(),
                "non-crack content opens through generic surface");
            const auto initialized = state->initialized;
            check(initialized > 0 && window_count() > original, "content initialized per display");
            check(
                surface.open([state]() { return std::make_unique<ProbeContent>(state); }).is_err(),
                "double open rejected without recreating content");
            auto drawn = surface.draw(effect::EffectTarget::all_displays());
            check(drawn.is_err() && state->draws == 1, "generic draw failure propagates");
            check(surface.hide(), "generic surface hides");
            surface.close();
            surface.close();
            check(state->destroyed == initialized && window_count() == original,
                "generic content and windows destroyed once");
            state->fail_initialize = true;
            check(
                surface.open([state]() { return std::make_unique<ProbeContent>(state); }).is_err(),
                "generic initialization failure propagates");
            check(state->destroyed == state->initialized && window_count() == original,
                "partial generic initialization releases resources");
            check(
                surface
                    .open([]() { return std::unique_ptr<platform_impl::IEffectSurfaceContent>{}; })
                    .is_err(),
                "null content rejected");
            check(window_count() == original, "null content cleanup releases windows");
            SDL_Quit();
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
#if defined(_WIN32)
        HWND native_window(SDL_Window* window) {
            return static_cast<HWND>(SDL_GetPointerProperty(
                SDL_GetWindowProperties(window), SDL_PROP_WINDOW_WIN32_HWND_POINTER, nullptr));
        }
        bool window_is_above(HWND first, HWND second) {
            check(first != nullptr && second != nullptr && first != second &&
                      IsWindow(first) != 0 && IsWindow(second) != 0,
                "Z-order comparison requires two distinct live windows");
            for (auto window = GetTopWindow(nullptr); window != nullptr;
                 window = GetWindow(window, GW_HWNDNEXT)) {
                if (window == first) {
                    return true;
                }
                if (window == second) {
                    return false;
                }
            }
            return false;
        }
#endif
        struct WindowSnapshot {
            SDL_WindowID id = 0;
            SDL_WindowFlags flags = 0;
            int x = 0;
            int y = 0;
            int width = 0;
            int height = 0;

            bool operator==(const WindowSnapshot&) const = default;
        };
        std::vector<WindowSnapshot> snapshot_windows() {
            int count = 0;
            std::unique_ptr<SDL_Window*, decltype(&SDL_free)> windows{
                SDL_GetWindows(&count), SDL_free};
            check(windows != nullptr, SDL_GetError());
            std::vector<WindowSnapshot> result;
            for (int index = 0; index < count; ++index) {
                auto* window = windows.get()[index];
                WindowSnapshot snapshot;
                snapshot.id = SDL_GetWindowID(window);
                snapshot.flags = SDL_GetWindowFlags(window);
                check(SDL_GetWindowPosition(window, &snapshot.x, &snapshot.y), SDL_GetError());
                check(SDL_GetWindowSize(window, &snapshot.width, &snapshot.height), SDL_GetError());
                result.push_back(snapshot);
            }
            std::sort(result.begin(), result.end(), [](const auto& left, const auto& right) {
                return left.id < right.id;
            });
            return result;
        }
        void check_overlay_layer(SDL_Window* model,
            effect::EffectLayer layer,
            SDL_DisplayID selected,
            int expected_visible) {
            int count = 0;
            std::unique_ptr<SDL_Window*, decltype(&SDL_free)> windows{
                SDL_GetWindows(&count), SDL_free};
            check(windows != nullptr, SDL_GetError());
            int visible = 0;
            for (int index = 0; index < count; ++index) {
                auto* window = windows.get()[index];
                if (window == model) {
                    continue;
                }
                const bool expected =
                    expected_visible > 0 &&
                    (selected == 0 || SDL_GetDisplayForWindow(window) == selected);
                check(((SDL_GetWindowFlags(window) & SDL_WINDOW_HIDDEN) == 0) == expected,
                    "only the requested displays are visible at each layer transition");
                if (!expected) {
                    continue;
                }
                ++visible;
                const bool foreground = layer == effect::EffectLayer::Foreground;
                check(((SDL_GetWindowFlags(window) & SDL_WINDOW_ALWAYS_ON_TOP) != 0) == foreground,
                    "overlay uses the requested stacking layer");
#if defined(_WIN32)
                check(foreground ? window_is_above(native_window(window), native_window(model))
                                 : window_is_above(native_window(model), native_window(window)),
                    "native Z order places the effect on the requested side of the model");
                RECT bounds{};
                const auto native = native_window(window);
                check(GetWindowRect(native, &bounds) != 0, "visible overlay has native bounds");
                // The hit-test point is the rectangle midpoint: origin + extent / 2.
                const POINT center{bounds.left + (bounds.right - bounds.left) / 2,
                    bounds.top + (bounds.bottom - bounds.top) / 2};
                check(WindowFromPoint(center) != native,
                    "layer transitions preserve overlay input transparency");
#endif
            }
            check(visible == expected_visible, "layer assertions cover every expected overlay");
        }
        void verify_layer_draw(effect::ICrackSurface& surface,
            SDL_Window* model,
            const effect::EffectTarget& target,
            SDL_DisplayID selected,
            int expected_visible) {
#if defined(_WIN32)
            const auto focus = GetForegroundWindow();
#endif
            auto drawn = surface.draw(1, std::nullopt, target);
            check(drawn.is_ok(), "layer draw succeeds");
            check_overlay_layer(model, target.layer, selected, expected_visible);
#if defined(_WIN32)
            check(GetForegroundWindow() == focus, "each layer draw preserves foreground focus");
#endif
        }
        void reject_invalid_layers(effect::ICrackSurface& surface) {
            const auto before = snapshot_windows();
            constexpr std::array INVALID_LAYERS{static_cast<effect::EffectLayer>(-1),
                static_cast<effect::EffectLayer>(2),
                static_cast<effect::EffectLayer>(std::numeric_limits<int>::max())};
            for (const auto layer : INVALID_LAYERS) {
#if defined(_WIN32)
                const auto focus = GetForegroundWindow();
#endif
                const auto rejected =
                    surface.draw(1, std::nullopt, effect::EffectTarget::all_displays(layer));
                check(rejected.is_err() &&
                          rejected.unwrap_err().get_error() == effect::EffectError::InvalidArgument,
                    "invalid layer returns InvalidArgument");
                check(snapshot_windows() == before,
                    "invalid layer preserves visibility, flags, identity and geometry");
#if defined(_WIN32)
                check(GetForegroundWindow() == focus, "invalid layer preserves foreground focus");
#endif
            }
        }
        void exercise_layer_transitions(effect::ICrackSurface& surface,
            SDL_Window* model,
            effect::EffectTarget target,
            SDL_DisplayID selected,
            int expected_visible) {
            constexpr std::array LAYERS{
                effect::EffectLayer::Foreground, effect::EffectLayer::Background};
            for (const auto initial : LAYERS) {
                check(surface.hide(), "hide before testing each initial layer");
                check_overlay_layer(model, initial, 0, 0);
                reject_invalid_layers(surface);
                target.layer = initial;
                verify_layer_draw(surface, model, target, selected, expected_visible);
                for (const auto next : LAYERS) {
                    target.layer = next;
                    constexpr int REPEATED_DRAWS = 2;
                    for (int repeat = 0; repeat < REPEATED_DRAWS; ++repeat) {
                        verify_layer_draw(surface, model, target, selected, expected_visible);
                        reject_invalid_layers(surface);
                        check_overlay_layer(model, next, selected, expected_visible);
                    }
                }
            }
        }
        void generic_surface_targeting() {
            check(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS), SDL_GetError());
            auto state = std::make_shared<ContentState>();
            state->fail_draw = false;
            platform_impl::EffectSurface surface;
            check(surface.hide(), "unopened generic surface hide is idempotent");
            check(surface.open([state]() { return std::make_unique<ProbeContent>(state); }).is_ok(),
                "generic content prepares without crack resources");
            int display_count = 0;
            std::unique_ptr<SDL_DisplayID, decltype(&SDL_free)> displays{
                SDL_GetDisplays(&display_count), SDL_free};
            check(displays != nullptr && display_count > 0, SDL_GetError());
            check(state->initialized == static_cast<unsigned>(display_count),
                "generic content initialized exactly once per display");
            check_visible_displays(0, 0);
            check(surface.draw(effect::EffectTarget::all_displays()).is_ok(),
                "generic content draws on all displays");
            check_visible_displays(0, display_count);
            check(state->frames.size() == static_cast<std::size_t>(display_count),
                "all-display draw visits every content exactly once");
            for (const auto& frame : state->frames) {
                check(frame.all_displays, "all-display frame carries the correct coordinate mode");
            }
            for (int index = 0; index < display_count; ++index) {
                SDL_Rect bounds{};
                check(SDL_GetDisplayBounds(displays.get()[index], &bounds), SDL_GetError());
                state->frames.clear();
                check(surface.draw(effect::EffectTarget::display_at(bounds.x, bounds.y)).is_ok(),
                    "generic content selects one display during playback");
                check_visible_displays(displays.get()[index], 1);
                check(state->frames.size() == 1, "unselected displays never draw their content");
                const auto& frame = state->frames.front();
                check(!frame.all_displays && frame.x == bounds.x && frame.y == bounds.y &&
                          frame.width == bounds.w && frame.height == bounds.h,
                    "selected content receives the exact logical display geometry");
            }
            const auto before = snapshot_windows();
            const auto draws = state->draws;
            auto invalid_kind = effect::EffectTarget::all_displays();
            invalid_kind.kind = static_cast<effect::EffectTargetKind>(-1);
            auto invalid_layer = effect::EffectTarget::all_displays();
            invalid_layer.layer = static_cast<effect::EffectLayer>(-1);
            for (const auto& invalid : {invalid_kind, invalid_layer}) {
                const auto result = surface.draw(invalid);
                check(result.is_err() &&
                          result.unwrap_err().get_error() == effect::EffectError::InvalidArgument,
                    "generic surface rejects invalid targets before invoking content");
                check(state->draws == draws && snapshot_windows() == before,
                    "invalid target changes neither content nor windows");
            }
            check(surface.hide(), "generic surface hides all content");
            check_visible_displays(0, 0);
            state->fail_draw = true;
            const auto failed = surface.draw(effect::EffectTarget::all_displays());
            check(failed.is_err() &&
                      failed.unwrap_err().get_error() == effect::EffectError::UpdateFailed &&
                      state->draws == draws + 1,
                "generic content failure stops drawing and propagates as UpdateFailed");
            check_visible_displays(0, 0);
            state->fail_draw = false;
            check(surface.draw(effect::EffectTarget::all_displays()).is_ok(),
                "generic content can draw again after a recoverable content error");
            check_visible_displays(0, display_count);
            surface.close();
            check(state->destroyed == state->initialized && window_count() == 0,
                "all generic content and windows are released");
            SDL_Quit();
        }
        void layered_overlays() {
            check(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS), SDL_GetError());
            {
                constexpr int MODEL_WINDOW_SIZE = 64;
                std::unique_ptr<SDL_Window, decltype(&SDL_DestroyWindow)> model{
                    SDL_CreateWindow("Model stacking fixture",
                        MODEL_WINDOW_SIZE,
                        MODEL_WINDOW_SIZE,
                        SDL_WINDOW_ALWAYS_ON_TOP | SDL_WINDOW_NOT_FOCUSABLE),
                    SDL_DestroyWindow};
                check(model != nullptr, SDL_GetError());
                auto shaders = std::make_shared<Shaders>();
                effect::CrackSettings settings;
                settings.mask_size = 64;
                auto mask = effect::make_crack_mask(settings);
                check(mask.is_ok(), "layer test mask generated");
                auto surface = platform_impl::create_crack_surface(shaders);
                check(surface->open(settings, std::move(mask.unwrap_mut())).is_ok(),
                    "layer surface prepared");
                const auto loads = shaders->loads;
                const auto prepared = snapshot_windows();
                int display_count = 0;
                std::unique_ptr<SDL_DisplayID, decltype(&SDL_free)> displays{
                    SDL_GetDisplays(&display_count), SDL_free};
                check(displays != nullptr && display_count > 0, SDL_GetError());
                check(window_count() == display_count + 1,
                    "exactly one overlay per display plus the model fixture");
                check_overlay_layer(model.get(), effect::EffectLayer::Foreground, 0, 0);
                check(surface->draw(1).is_ok(), "default effect layer draws");
                check_overlay_layer(model.get(), effect::EffectLayer::Foreground, 0, display_count);
#if defined(_WIN32)
                check(SetWindowPos(native_window(model.get()),
                          HWND_TOPMOST,
                          0,
                          0,
                          0,
                          0,
                          SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE) != 0,
                    "simulate model being raised by a click");
                // Prove the perturbation happened before checking that drawing repairs it.
                for (const auto& snapshot : prepared) {
                    if (snapshot.id != SDL_GetWindowID(model.get())) {
                        check(window_is_above(native_window(model.get()),
                                  native_window(SDL_GetWindowFromID(snapshot.id))),
                            "fixture actually places the model above every overlay");
                    }
                }
#endif
                verify_layer_draw(
                    *surface, model.get(), effect::EffectTarget::all_displays(), 0, display_count);
                exercise_layer_transitions(
                    *surface, model.get(), effect::EffectTarget::all_displays(), 0, display_count);
                for (int index = 0; index < display_count; ++index) {
                    SDL_Rect bounds{};
                    check(SDL_GetDisplayBounds(displays.get()[index], &bounds), SDL_GetError());
                    exercise_layer_transitions(*surface,
                        model.get(),
                        effect::EffectTarget::display_at(bounds.x, bounds.y),
                        displays.get()[index],
                        1);
                }
                verify_layer_draw(
                    *surface, model.get(), effect::EffectTarget::all_displays(), 0, display_count);
                check(surface->hide() && surface->hide(), "repeated hide succeeds");
                check_overlay_layer(model.get(), effect::EffectLayer::Foreground, 0, 0);
                const auto after = snapshot_windows();
                check(after.size() == prepared.size() && shaders->loads == loads,
                    "layer changes do not create windows or reload shaders");
                for (std::size_t index = 0; index < prepared.size(); ++index) {
                    check(after[index].id == prepared[index].id &&
                              after[index].x == prepared[index].x &&
                              after[index].y == prepared[index].y &&
                              after[index].width == prepared[index].width &&
                              after[index].height == prepared[index].height,
                        "layer transitions preserve each window's identity and geometry");
                }
                surface->close();
                surface->close();
                check(window_count() == 1 && SDL_GetWindowID(model.get()) != 0,
                    "closing overlays preserves the model window");
                check(surface->draw(1).is_err(), "closed surface rejects drawing");
            }
            check(window_count() == 0, "layer test resources released");
            SDL_Quit();
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
    enishi::tests::run_test(
        "Generic effect surface lifecycle", enishi::tests::generic_surface_lifecycle);
    enishi::tests::run_test(
        "Generic effect surface targeting", enishi::tests::generic_surface_targeting);
    enishi::tests::run_test("SDL GL context restoration", enishi::tests::context_restoration);
    enishi::tests::run_test("Crack surface input contract", enishi::tests::surface_contract);
    enishi::tests::run_test("Prepared crack renderer reuse", enishi::tests::prepared_overlays);
    enishi::tests::run_test("Effect foreground and background", enishi::tests::layered_overlays);
    enishi::tests::run_test("Crack display targeting", enishi::tests::targeted_overlays);
    enishi::tests::run_test("Crack overlay renderer lifecycle", enishi::tests::overlays);
}
