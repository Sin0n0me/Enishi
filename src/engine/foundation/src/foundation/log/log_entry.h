#pragma once
#include "../str/str.h"
#include "log_level.h"
#include <chrono>

namespace enishi ::foundation {
    struct LogEntry {
        LogLevel level;
        UTF8 message;
        std::chrono::system_clock::time_point time;

        UTF8 format_log(void) const;
    };
} // namespace enishi::foundation