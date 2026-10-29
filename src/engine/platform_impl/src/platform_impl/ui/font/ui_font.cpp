#include "ui_font.h"
#include <debug/log/logger.h>
#include <filesystem>
#include <imgui.h>
#if defined(_WIN32)
#include "font_paths_windows.h"
#elif defined(__APPLE__)
#include "font_paths_macos.h"
#else
#include "font_paths_linux.h"
#endif

namespace enishi::platform_impl {
    void load_ui_font(void) {
        constexpr float FONT_SIZE = 18.0f;
        const auto candidates = system_font_candidates();
        for (const auto& path : candidates) {
            std::error_code error;
            if (!std::filesystem::is_regular_file(path, error)) {
                continue;
            }
            const auto utf8 = path.u8string();
            const auto filename = std::string(utf8.begin(), utf8.end());
            if (ImGui::GetIO().Fonts->AddFontFromFileTTF(filename.c_str(), FONT_SIZE) != nullptr) {
                return;
            }
        }
        debug::Logger::warning("No Japanese UI font is available; using the default font");
    }
} // namespace enishi::platform_impl
