#pragma once
#include <filesystem>
#include <vector>
namespace enishi::platform_impl {
    std::vector<std::filesystem::path> system_font_candidates();
}
