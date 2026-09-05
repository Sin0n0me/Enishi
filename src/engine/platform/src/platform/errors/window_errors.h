#pragma once
#include <string>

namespace enishi::platform {
    enum class WindowError {
        FailedMakeWindow,
        FailedSetWinodwSize,
        FailedSetWinodwPosition,
        FailedSetWinodwTitle,
        InitError,
    };
} // namespace enishi::platform
