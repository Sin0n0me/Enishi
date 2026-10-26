#pragma once
#include <glm/vec2.hpp>

namespace enishi::debug {
    // Normalized viewport coordinates, with the origin at the top left.
    struct OverlayLine {
        glm::vec2 start;
        glm::vec2 end;
    };
} // namespace enishi::debug
