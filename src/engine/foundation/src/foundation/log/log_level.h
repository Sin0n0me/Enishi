#pragma once
#include "../str/str.h"

namespace enishi ::foundation {
    enum class LogLevel {
        Debug,    //
        Info,     //
        Warning,  //
        Error,    // 致命的ではるが場合により回復可能な場合
        Critical, // 継続できないような致命的な場合
    };

    const UTF8 level_to_string(const LogLevel& level);
} // namespace enishi::foundation