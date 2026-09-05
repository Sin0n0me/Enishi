#pragma once
#include "clip_data/animation.h"
#include "clip_data/interpolation/interpolation.h"
#include <glm/gtc/quaternion.hpp>

namespace enishi::animation {
    // フィールドは持たずに純粋なロジックのみを持つ
    class KeyframeInterpolator {
      public:
        template <typename T>
        [[nodiscard]]
        static T sample(const Keyframes<T>& keyframes, const float time) {
            // キーフレームが1つだけの場合
            if (keyframes.times.size() == 1) {
                return keyframes.values[0];
            }

            // 時刻が範囲外の場合はクランプ
            if (time <= keyframes.times.front()) {
                return keyframes.values.front();
            }
            if (time >= keyframes.times.back()) {
                return keyframes.values.back();
            }

            // time を挟む2つのキーフレームを二分探索で探す
            const auto it = std::lower_bound(keyframes.times.begin(), keyframes.times.end(), time);
            const std::uint32_t next_index =
                static_cast<std::uint32_t>(it - keyframes.times.begin());
            const std::uint32_t prev_index = next_index - 1;

            const float prev_time = keyframes.times[prev_index];
            const float next_time = keyframes.times[next_index];

            // 何かしらが原因で差がない場合は前回フレームの値を返す
            // ログで警告を出してもいいかも
            const float time_diff = next_time - prev_time;
            if (time_diff == 0.0) {
                return keyframes.values[prev_index];
            }

            // 区間内での正規化された時刻 [0.0, 1.0]
            const float t = (time - prev_time) / time_diff;

            switch (keyframes.interpolation) {
                case InterpolationType::Step:
                    return keyframes.values[prev_index];
                case InterpolationType::Linear:
                    return LinearInterpolation::lerp(
                        keyframes.values[prev_index], keyframes.values[next_index], t);
                case InterpolationType::CubicSpline:
                    return Hermite::sample_hermite(keyframes, prev_index, t, time_diff);
                case InterpolationType::VmdBezier:
                    return VMDInterpolation::sample_vmd_bezier(keyframes, prev_index, t);
            }

            return keyframes.values[prev_index];
        }
    };
} // namespace enishi::animation
