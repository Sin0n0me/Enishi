#pragma once
#include <cstdint>
namespace enishi::effect {
    struct CrackSettings {
        std::uint32_t seed = 1;
        std::uint32_t mask_size = 512;
        std::uint32_t cells = 8;
        // Width and feather are in Voronoi cell units; depth is in texture UV units.
        float width = 0.025f;
        float feather = 0.012f;
        float depth = 0.018f;
        // Tangent-space view direction divided by its forward component.
        float view_x = 0.25f;
        float view_y = -0.15f;
        float duration_seconds = 3.0f;
        float fade_seconds = 0.5f;
        float opacity = 0.9f;
    };
} // namespace enishi::effect
