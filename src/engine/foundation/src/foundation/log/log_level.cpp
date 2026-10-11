#include "log_level.h"

namespace enishi ::foundation {
    const UTF8 level_to_string(const LogLevel& level) {
        switch (level) {
            case LogLevel::Debug:
                return "Debug";
            case LogLevel::Info:
                return "Info";
            case LogLevel::Warning:
                return "Warning";
            case LogLevel::Error:
                return "Error";
            case LogLevel::Critical:
                return "Critical";
        }
        return "Unknown";
    }
} // namespace enishi::foundation