#include "ray.h"

namespace enishi::types {
    Ray Ray::make_ray_from_screen(const WindowSize& window_size,
        const ClientMousePosition& mouse_position,
        const UniformCamera& camera) {
        if (!(window_size.width > 0) || !(window_size.height > 0)) {
            return {glm::vec3(0), glm::vec3(0)};
        }
        constexpr float NDC_SCALE = 2.0f;
        constexpr float FAR_DEPTH = 1.0f;
        // Client pixels -> NDC: x = 2*x/width - 1, y = 1 - 2*y/height.
        const float x = NDC_SCALE * static_cast<float>(mouse_position.x) /
                            static_cast<float>(window_size.width) -
                        1.0f;
        const float y = 1.0f - NDC_SCALE * static_cast<float>(mouse_position.y) /
                                   static_cast<float>(window_size.height);
        // p_world = inverse(P * V) * p_clip, followed by the homogeneous divide.
        const auto far_point =
            glm::inverse(camera.projection * camera.view) * glm::vec4(x, y, FAR_DEPTH, 1);
        const auto origin = glm::vec3(glm::inverse(camera.view)[3]);
        if (far_point.w == 0.0f) {
            return {origin, glm::vec3(0)};
        }
        const auto direction = glm::vec3(far_point) / far_point.w - origin;
        if (!(glm::dot(direction, direction) > 0.0f)) {
            return {origin, glm::vec3(0)};
        }
        return {origin, glm::normalize(direction)};
    }
} // namespace enishi::types
