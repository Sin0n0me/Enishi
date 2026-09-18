#pragma once
#include "../str/str.h"
#include <filesystem>

namespace enishi::foundation {
    UTF8 path_to_regex_str(const std::filesystem::path& path);
} // namespace enishi::foundation