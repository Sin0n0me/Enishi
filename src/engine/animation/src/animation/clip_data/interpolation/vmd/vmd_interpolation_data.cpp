#pragma once
#include "vmd_interpolation_data.h"

namespace enishi::animation {
    VMDBezier VMDBezier::make_animation_bezier(
        const std::array<std::uint8_t, 64>& interpolation, const size_t offset) {
        const std::uint8_t x0 = interpolation[0 + offset];
        const std::uint8_t y0 = interpolation[4 + offset];
        const std::uint8_t x1 = interpolation[8 + offset];
        const std::uint8_t y1 = interpolation[12 + offset];

        return VMDBezier{
            .x1 = static_cast<float>(x0) / 127.0f,
            .y1 = static_cast<float>(y0) / 127.0f,
            .x2 = static_cast<float>(x1) / 127.0f,
            .y2 = static_cast<float>(y1) / 127.0f,
        };
    }

    VMDBezier VMDBezier::make_camera_bezier(
        const std::array<std::uint8_t, 64>& interpolation, const size_t offset) {
        const std::uint8_t x0 = interpolation[0 + offset];
        const std::uint8_t y0 = interpolation[1 + offset];
        const std::uint8_t x1 = interpolation[2 + offset];
        const std::uint8_t y1 = interpolation[3 + offset];

        return VMDBezier{
            .x1 = static_cast<float>(x0) / 127.0f,
            .y1 = static_cast<float>(y0) / 127.0f,
            .x2 = static_cast<float>(x1) / 127.0f,
            .y2 = static_cast<float>(y1) / 127.0f,
        };
    }

    float VMDBezier::eval_x(const float t) const {
        const float it = 1.0f - t;
        const float it2 = it * it;
        const float t2 = t * t;
        return t2 * t * 1.0f + 3.0f * t2 * it * this->x2 + 3.0f * t * it2 * this->x1 +
               it2 * it * 0.0f;
    }

    float VMDBezier::eval_y(const float t) const {
        const float it = 1.0f - t;
        const float it2 = it * it;
        const float t2 = t * t;
        return t2 * t * 1.0f + 3.0f * t2 * it * this->y2 + 3.0f * t * it2 * this->y1 +
               it2 * it * 0.0f;
    }

    float VMDBezier::find_bezier_x(const float t) const {
        constexpr float EPSILON = 1e-5f;
        float start = 0.0f;
        float stop = 1.0f;
        float t2 = 0.5f;
        for (float x = this->eval_x(t2); EPSILON < std::abs(t - x);) {
            if (t < x) {
                stop = t2;
            } else {
                start = t2;
            }
            t2 = (stop + start) * 0.5f;
            x = this->eval_x(t2);
        }

        return t2;
    }

    VMDAnimationBezier VMDAnimationBezier::make(const std::array<std::uint8_t, 64>& interpolation) {
        return VMDAnimationBezier{
            .translate_x_bezier = VMDBezier::make_animation_bezier(interpolation, 0),
            .translate_y_bezier = VMDBezier::make_animation_bezier(interpolation, 1),
            .translate_z_bezier = VMDBezier::make_animation_bezier(interpolation, 2),
            .rotate_bezier = VMDBezier::make_animation_bezier(interpolation, 3),
        };
    }
} // namespace enishi::animation