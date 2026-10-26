#pragma once

namespace enishi::debug {
#if !defined(NDEBUG) || defined(_DEBUG) || defined(DEBUG)
    constexpr bool IS_DEBUG_MODE = true;
#else
    constexpr bool IS_DEBUG_MODE = false;
#endif
} // namespace enishi::debug
