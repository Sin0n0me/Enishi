#pragma once
#include "../../tracks.h"
#include "hermite_data.h"
#include <cmath>

namespace enishi::animation {
    class Hermite {
      public:
        template <typename T>
        [[nodiscard]]
        static constexpr T sample_hermite(const Keyframes<T>& key_frames,
            const std::uint32_t prev_index,
            const float u,
            const float dt) {
            const auto& data = std::get<HermiteTangents<T>>(key_frames.interpolation[prev_index]);
            const auto& next_data =
                std::get<HermiteTangents<T>>(key_frames.interpolation[prev_index + 1]);
            return Hermite::cubic_spline(key_frames.values[prev_index],
                data.out_tangent,
                key_frames.values[prev_index + 1],
                next_data.in_tangent,
                u,
                dt);
        }

        // 三次エルミートスプライン補間
        template <typename T>
        [[nodiscard]]
        static constexpr T cubic_spline(const T& p0,
            const T& m0,
            const T& p1,
            const T& m1,
            const float t,
            const float dt) noexcept {
            const float t2 = t * t;
            const float t3 = t2 * t;
            // エルミート基底関数
            return (2.0f * t3 - 3.0f * t2 + 1.0f) * p0 + (t3 - 2.0f * t2 + t) * dt * m0 +
                   (-2.0f * t3 + 3.0f * t2) * p1 + (t3 - t2) * dt * m1;
        }
    };
} // namespace enishi::animation