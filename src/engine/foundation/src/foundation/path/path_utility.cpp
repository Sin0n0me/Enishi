#include "path_utility.h"

namespace enishi::foundation {
    UTF8 path_to_regex_str(const std::filesystem::path& path) {
        constexpr std::string_view META_CHARS = R"(\.^$|()[]{}*+?-)";
        UTF8 result;
        const auto str = path.lexically_normal().string<char>();
        result.reserve(str.size() * 2);

        for (const char c : str) {
            if (META_CHARS.contains(c)) {
                result += '\\';
            }
            result += c;
        }

        return result;
    }
} // namespace enishi::foundation