#pragma once
#include "../../tracks.h"
#include "../linear_interpolation/linear_interpolation.h"
#include "vmd_interpolation_data.h"

namespace enishi::animation {
    class VMDInterpolation {
      public:
        template <typename T>
        [[nodiscard]]
        static T sample_vmd_bezier(
            const Keyframes<T>& keyframes, const std::uint32_t prev_index, const float u) {
            const auto& bezier = std::get<VMDAnimationBezier>(keyframes.interpolation[prev_index]);
            return VMDAnimationBezier::lerp(
                bezier, keyframes.values[prev_index], keyframes.values[prev_index + 1], u);
        }

      private:
        template <typename T>
        [[nodiscard]]
        static T lerp(
            const VMDAnimationBezier& bezier, const T& prev, const T& next, float u) noexcept;

        template <>
        [[nodiscard]]
        static glm::vec3 lerp<glm::vec3>(const VMDAnimationBezier& bezier,
            const glm::vec3& prev,
            const glm::vec3& next,
            const float u) noexcept {
            const float tx_x = bezier.translate_x_bezier.find_bezier_x(u);
            const float ty_x = bezier.translate_y_bezier.find_bezier_x(u);
            const float tz_x = bezier.translate_z_bezier.find_bezier_x(u);
            const float tx_y = bezier.translate_x_bezier.eval_y(tx_x);
            const float ty_y = bezier.translate_y_bezier.eval_y(ty_x);
            const float tz_y = bezier.translate_z_bezier.eval_y(tz_x);

            // 平行移動成分の線形補間
            // pT: 前フレームの平行移動成分
            // cT: 次フレームの平行移動成分
            // t: 各成分に対しての補間係数
            // T = pT * (1 - t) + cT * t
            const glm::vec3 t = glm::vec3{tx_y, ty_y, tz_y};
            const glm::vec3 it = glm::vec3{1.0f, 1.0f, 1.0f} - t;
            return prev * it + next * t;
        }

        template <>
        [[nodiscard]]
        static glm::quat lerp<glm::quat>(const VMDAnimationBezier& bezier,
            const glm::quat& prev,
            const glm::quat& next,
            const float u) noexcept {
            const float rotate_x = bezier.rotate_bezier.find_bezier_x(u);
            const float rotate_y = bezier.rotate_bezier.eval_y(rotate_x);
            return LinearInterpolation::lerp(prev, next, rotate_y);
        }
    };
} // namespace enishi::animation