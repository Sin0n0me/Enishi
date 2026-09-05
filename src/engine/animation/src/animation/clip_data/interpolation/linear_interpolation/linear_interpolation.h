#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

namespace enishi::animation {
    class LinearInterpolation {
      public:
        // vec3 の線形補間
        [[nodiscard]]
        static constexpr glm::vec3 lerp(
            const glm::vec3& a, const glm::vec3& b, const float t) noexcept {
            return glm::mix(a, b, t);
        }

        // quat は slerp で補間(vecと同じ線形補間すると回転が歪む)
        [[nodiscard]]
        static glm::quat lerp(const glm::quat& a, const glm::quat& b, const float t) noexcept {
            return glm::slerp(a, b, t);
        }

        // float の線形補間(モーフウェイト用)
        [[nodiscard]]
        static float lerp(const float a, const float b, const float t) noexcept {
            return a + (b - a) * t;
        }
    };
} // namespace enishi::animation