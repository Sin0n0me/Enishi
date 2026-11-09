#include "font_paths_macos.h"
namespace enishi::platform_impl {
    std::vector<std::filesystem::path> system_font_candidates() {
        return {std::filesystem::path(u8"/System/Library/Fonts/ヒラギノ角ゴシック W3.ttc"),
            "/System/Library/Fonts/Supplemental/Arial Unicode.ttf"};
    }
} // namespace enishi::platform_impl
