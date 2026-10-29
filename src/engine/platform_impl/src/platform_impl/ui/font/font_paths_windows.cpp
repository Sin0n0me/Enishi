#include "font_paths_windows.h"
#include <Windows.h>
#include <array>
namespace enishi::platform_impl {
    std::vector<std::filesystem::path> system_font_candidates() {
        std::array<wchar_t, MAX_PATH> directory{};
        const auto size =
            GetWindowsDirectoryW(directory.data(), static_cast<UINT>(directory.size()));
        if (size == 0 || !(size < directory.size())) {
            return {};
        }
        const auto fonts = std::filesystem::path(directory.data()) / "Fonts";
        return {fonts / "YuGothM.ttc", fonts / "meiryo.ttc", fonts / "msgothic.ttc"};
    }
} // namespace enishi::platform_impl
