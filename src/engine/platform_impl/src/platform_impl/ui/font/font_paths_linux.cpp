#include "font_paths_linux.h"
namespace enishi::platform_impl {
    std::vector<std::filesystem::path> system_font_candidates() {
        return {"/usr/share/fonts/opentype/noto/NotoSansCJK-Regular.ttc",
            "/usr/share/fonts/truetype/noto/NotoSansCJK-Regular.ttc"};
    }
} // namespace enishi::platform_impl
