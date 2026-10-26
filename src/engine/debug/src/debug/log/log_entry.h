#pragma once
#include "log_level.h"
#include <chrono>
#include <foundation/str/str.h>

namespace enishi::debug {
    struct LogEntry {
        LogLevel level;
        foundation::UTF8 message;
        std::chrono::system_clock::time_point time;

        foundation::UTF8 format_log(void) const;
    };
} // namespace enishi::debug