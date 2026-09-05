#include "log_entry.h"
#include <format>
#include <ranges>
#include <string_view>

namespace enishi ::foundation {
    UTF8 add_indent(const UTF8& input, UTF8 prefix) {
        auto lines = input | std::views::split('\n');
        UTF8 result;

        for (auto it = lines.begin(); it != lines.end(); ++it) {
            const UTF8 line{(*it).data(), (*it).size()};

            if (it != lines.begin()) {
                result += prefix;
            }
            result += line;
            result += '\n';
        }
        return result;
    }

    UTF8 LogEntry::format_log(void) const {
        // https://cpprefjp.github.io/reference/chrono/format.html
        const UTF8 time = std::format("[{:%F %T}]", this->time);
        const UTF8 time_stamp(time.begin(), time.end());
        const UTF8 log_level = "[" + level_to_string(this->level) + "]";
        const UTF8 indent(time_stamp.length() + log_level.length() + 2, ' ');

        return time_stamp + " " + log_level + " " + add_indent(this->message, indent);
    }
} // namespace enishi::foundation