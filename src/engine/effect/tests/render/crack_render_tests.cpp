#include "crack_render_tests.h"
#include "../../../core/tests/test_runner.h"
#include <SDL3/SDL.h>
#include <effect/crack/crack_effect.h>
#include <fstream>
#include <iterator>
#if defined(_WIN32)
#include <Windows.h>
#endif
#include <platform_impl/effect/crack_surface.h>
#include <platform_impl/renderer/renderer_factory.h>
namespace enishi::tests {
    namespace {
        class Shaders final : public platform::IShaderDataProvider {
          public:
            bool force_failure = false;

          public:
            foundation::Result<std::vector<platform::ShaderDataEntry>, platform::RenderError> get(
                std::span<const std::tuple<types::ShaderKind, std::filesystem::path>> paths)
                const override;
        };
        foundation::Result<std::vector<platform::ShaderDataEntry>, platform::RenderError>
        Shaders::get(
            std::span<const std::tuple<types::ShaderKind, std::filesystem::path>> paths) const {
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
                    auto started = effect->start();
                    if (started.is_err()) {
                        check(false, started.unwrap_err().get_message().c_str());
                    }
                    auto updated = effect->update(types::DeltaTime{std::chrono::milliseconds{16}});
                    if (updated.is_err()) {
                        check(false, updated.unwrap_err().get_message().c_str());
                    }
                }
                check(window_count() > original, "effects own display overlays");
                input_transparency();
                first.stop();
                second.stop();
                check(window_count() == original, "all overlay windows released");
                check((SDL_WasInit(SDL_INIT_VIDEO) & SDL_INIT_VIDEO) != 0,
                    "effects preserve application SDL initialization");
            }
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
    enishi::tests::run_test("Crack overlay renderer lifecycle", enishi::tests::overlays);
}
