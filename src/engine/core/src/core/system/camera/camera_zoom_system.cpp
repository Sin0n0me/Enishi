#include "camera_zoom_system.h"
#include <cmath>
#include <debug/log/logger.h>
#include <glm/gtc/matrix_transform.hpp>
#include <utility>

namespace enishi::core {
    CameraZoomSystem::CameraZoomSystem(std::shared_ptr<platform::IWindow> window,
        std::shared_ptr<sub_system::ICameraController> camera,
        CameraZoomConfig config)
        : window(std::move(window))
        , camera(std::move(camera))
        , config(config) {
    }

    void CameraZoomSystem::pre_update() {
        if (this->window == nullptr || this->camera == nullptr) {
            return;
        }
        const auto input = this->window->get_input();
        if (input.is_none()) {
            return;
        }
        const float wheel = input.unwrap()->get_mouse_wheel().y;
        if (!std::isfinite(wheel) || wheel == 0) {
            return;
        }
        constexpr float DISTANCE_PER_TICK = 1.0f;
        const float sign = this->config.invert_wheel ? -1.0f : 1.0f;
        const float distance = wheel * sign * DISTANCE_PER_TICK;
        const auto snapshot = this->camera->get_camera();
        // Left-handed camera faces local +Z. Moving the camera by +d along its forward axis
        // translates world points by -d in view space: V_new = T(0,0,-d) * V_old.
        const auto view = glm::translate(glm::mat4(1), glm::vec3(0, 0, -distance)) * snapshot.view;
        const auto result = this->camera->set_view_projection(view, snapshot.projection);
        if (result.is_err()) {
            debug::Logger::warning(result.unwrap_err().get_message());
        }
    }

    void CameraZoomSystem::update(const types::DeltaTime& delta_time) {
    }
    void CameraZoomSystem::post_update() {
    }
    void CameraZoomSystem::render() const {
    }
} // namespace enishi::core
