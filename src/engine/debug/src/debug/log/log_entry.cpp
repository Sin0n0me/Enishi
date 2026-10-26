#include "log_entry.h"
#include <format>
#include <ranges>
#include <string_view>

namespace enishi::debug {
    foundation::UTF8 add_indent(const foundation::UTF8& input, foundation::UTF8 prefix) {
        auto lines = input | std::views::split('\n');
        foundation::UTF8 result;

        for (auto it = lines.begin(); it != lines.end(); ++it) {
            const foundation::UTF8 line{(*it).data(), (*it).size()};

            if (it != lines.begin()) {
                result += prefix;
            }
            result += line;
            result += '\n';
        }
        return result;
    }

    foundation::UTF8 LogEntry::format_log(void) const {
        // https://cpprefjp.github.io/reference/chrono/format.html
        const foundation::UTF8 time = std::format("[{:%F %T}]", this->time);
        const foundation::UTF8 time_stamp(time.begin(), time.end());
        const foundation::UTF8 log_level = "[" + level_to_string(this->level) + "]";
        const foundation::UTF8 indent(time_stamp.length() + log_level.length() + 2, ' ');

        return time_stamp + " " + log_level + " " + add_indent(this->message, indent);
    }
} // namespace enishi::debug