#pragma once
#include "../../tracks.h"
#include "../linear_interpolation/linear_interpolation.h"
#include "vmd_interpolation_data.h"
#include <type_traits>

namespace enishi::animation {
    class VMDInterpolation {
      public:
        template <typename T>
        [[nodiscard]] static T sample_vmd_bezier(
            const Keyframes<T>& keyframes, const std::uint32_t prev_index, const float u) {
            const auto& bezier = std::get<VMDAnimationBezier>(keyframes.interpolation[prev_index]);
            if constexpr (std::is_same_v<T, glm::vec3>) {
                return VMDInterpolation::lerp(bezier,
                    keyframes.values[prev_index],
                    keyframes.values[prev_index + 1],
                    u);
            } else {
                return VMDInterpolation::lerp(bezier,
                    keyframes.values[prev_index],
                    keyframes.values[prev_index + 1],
                    u);
            }
        }

      private:
        [[nodiscard]] static glm::vec3 lerp(const VMDAnimationBezier& bezier,
            const glm::vec3& prev,
            const glm::vec3& next,
            const float u) noexcept {
            const float tx = bezier.translate_x_bezier.eval_y(
                bezier.translate_x_bezier.find_bezier_x(u));
            const float ty = bezier.translate_y_bezier.eval_y(
                bezier.translate_y_bezier.find_bezier_x(u));
            const float tz = bezier.translate_z_bezier.eval_y(
                bezier.translate_z_bezier.find_bezier_x(u));
            return prev * (glm::vec3(1.0f) - glm::vec3(tx, ty, tz)) + next * glm::vec3(tx, ty, tz);
        }

        [[nodiscard]] static glm::quat lerp(const VMDAnimationBezier& bezier,
            const glm::quat& prev,
            const glm::quat& next,
            const float u) noexcept {
            const float t = bezier.rotate_bezier.eval_y(bezier.rotate_bezier.find_bezier_x(u));
            return LinearInterpolation::lerp(prev, next, t);
        }
    };
} // namespace enishi::animation
