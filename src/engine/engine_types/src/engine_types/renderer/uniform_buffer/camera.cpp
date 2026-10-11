#include "camera.h"
#include <glm/gtc/matrix_transform.hpp>

namespace enishi::types {
    UniformCamera make_default_model_camera(void) {
        constexpr float TARGET_HEIGHT = 11.0f;
        constexpr float EYE_HEIGHT_OFFSET = 7.5f;
        constexpr float DISTANCE = 25.0f;
        constexpr float FIELD_OF_VIEW_DEGREES = 45.0f;
        constexpr float VIEWPORT_WIDTH = 600.0f;
        constexpr float VIEWPORT_HEIGHT = 800.0f;
        constexpr float NEAR_PLANE = 0.1f;
        constexpr float FAR_PLANE = 200.0f;
        const glm::vec3 eye(0, TARGET_HEIGHT + EYE_HEIGHT_OFFSET, -DISTANCE);
        const glm::vec3 target(0, TARGET_HEIGHT, 0);
        const glm::vec3 up(0, 1, 0);
        UniformCamera camera{
            .world = glm::mat4(1),
            .view = glm::lookAtLH(eye, target, up),
            .projection = glm::perspectiveFovLH(glm::radians(FIELD_OF_VIEW_DEGREES),
                VIEWPORT_WIDTH,
                VIEWPORT_HEIGHT,
                NEAR_PLANE,
                FAR_PLANE),
        };
        // Column-vector convention: clip = projection * view * world * position.
        camera.mvp = camera.projection * camera.view * camera.world;
        return camera;
    }
} // namespace enishi::types
