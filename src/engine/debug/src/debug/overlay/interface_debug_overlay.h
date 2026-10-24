#pragma once
#include "overlay_line.h"
#include <span>

namespace enishi::debug {
    class IDebugOverlay {
      public:
        virtual ~IDebugOverlay() noexcept = default;
        // Submit after begin_frame and before end_frame on the UI thread.
        virtual void draw_lines(std::span<const OverlayLine> lines) = 0;
    };
} // namespace enishi::debug
