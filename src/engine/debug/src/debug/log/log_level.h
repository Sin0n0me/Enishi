#pragma once
#include <foundation/str/str.h>

namespace enishi::debug {
    enum class LogLevel {
        Debug,    //
        Info,     //
        Warning,  //
        Error,    // 致命的ではるが場合により回復可能な場合
        Critical, // 継続できないような致命的な場合
    };

    const foundation::UTF8 level_to_string(const LogLevel& level);
} // namespace enishi::debug