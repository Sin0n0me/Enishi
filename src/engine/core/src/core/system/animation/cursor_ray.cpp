#include "cursor_ray.h"
#include <platform/window/interface_window.h>

namespace enishi::core {
    std::optional<animation::CursorRay> read_cursor_ray(
        const platform::IWindow& window, const types::UniformCamera& camera, bool allow_outside) {
        const auto input = window.get_input();
        const auto position = window.get_position();
        const auto size = window.get_size();
        if (input.is_none() || position.is_none() || size.is_none()) {
            return std::nullopt;
        }
        const auto cursor = input.unwrap()->get_client_mouse_position(position.unwrap());
        if (!allow_outside && (cursor.x < 0 || cursor.y < 0 || !(cursor.x < size.unwrap().width) ||
                                  !(cursor.y < size.unwrap().height))) {
            return std::nullopt;
        }
        const auto ray = types::Ray::make_ray_from_screen(size.unwrap(), cursor, camera);
        const auto inverse_world = glm::inverse(camera.world);
        // Convert both the picking ray and the camera-facing target plane to model space.
        return animation::CursorRay{{glm::vec3(inverse_world * glm::vec4(ray.origin, 1)),
                                        glm::normalize(glm::mat3(inverse_world) * ray.direction)},
            glm::normalize(glm::mat3(inverse_world) * glm::vec3(glm::inverse(camera.view)[2]))};
    }
} // namespace enishi::core
