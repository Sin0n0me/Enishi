#pragma once
#include "../linear_interpolation/linear_interpolation.h"
#include <array>
#include <assets_system/animation/vmd/vmd_data.h>
#include <assets_system/model/interface_bone_resolver.h>

namespace enishi::animation {
    struct VMDBezier {
        float x1;
        float y1;
        float x2;
        float y2;

        static VMDBezier make_animation_bezier(
            const std::array<std::uint8_t, 64>& interpolation, const size_t offset);
        static VMDBezier make_camera_bezier(
            const std::array<std::uint8_t, 64>& interpolation, const size_t offset);

        float eval_x(const float t) const;
        float eval_y(const float t) const;
        float find_bezier_x(const float t) const;
    };

    struct VMDAnimationBezier {
        VMDBezier translate_x_bezier;
        VMDBezier translate_y_bezier;
        VMDBezier translate_z_bezier;
        VMDBezier rotate_bezier;

        static VMDAnimationBezier make(const std::array<std::uint8_t, 64>& interpolation);
    };
} // namespace enishi::animation