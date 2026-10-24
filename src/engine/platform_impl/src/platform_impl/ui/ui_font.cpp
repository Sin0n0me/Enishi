#include "ui_font.h"
#include <array>
#include <filesystem>
#include <debug/log/logger.h>
#include <imgui.h>
#if defined(_WIN32)
#include <Windows.h>
#endif

namespace enishi::platform_impl {
    void load_ui_font(void) {
        constexpr float FONT_SIZE = 18.0f;
#if defined(_WIN32)
        std::array<wchar_t, MAX_PATH> directory{};
        const auto size =
            GetWindowsDirectoryW(directory.data(), static_cast<UINT>(directory.size()));
        if (size == 0 || !(size < directory.size())) {
            debug::Logger::warning("Unable to locate the system font directory");
            return;
        }
        const auto font_directory = std::filesystem::path(directory.data()) / "Fonts";
        const std::array candidates{font_directory / "YuGothM.ttc",
            font_directory / "meiryo.ttc",
            font_directory / "msgothic.ttc"};
#else
        const std::array candidates{
            std::filesystem::path("/usr/share/fonts/opentype/noto/NotoSansCJK-Regular.ttc"),
            std::filesystem::path("/usr/share/fonts/truetype/noto/NotoSansCJK-Regular.ttc")};
#endif
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
