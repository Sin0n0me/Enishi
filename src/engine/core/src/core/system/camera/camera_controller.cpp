#include "camera_controller.h"
#include <cmath>

namespace enishi::core {
    namespace {
        bool finite_matrix(const glm::mat4& matrix) {
            for (glm::length_t column = 0; column < matrix.length(); ++column) {
                for (glm::length_t row = 0; row < matrix[column].length(); ++row) {
                    if (!std::isfinite(matrix[column][row])) {
                        return false;
                    }
                }
            }
            return true;
        }

        bool invertible(const glm::mat4& matrix) {
            const float determinant = glm::determinant(matrix);
            return finite_matrix(matrix) && std::isfinite(determinant) && determinant != 0 &&
                   finite_matrix(glm::inverse(matrix));
        }
    } // namespace

    CameraController::CameraController()
        : camera(types::make_default_model_camera()) {
    }

    types::UniformCamera CameraController::get_camera() const {
        return this->camera;
    }

    foundation::VoidResult<sub_system::CameraError> CameraController::set_view_projection(
        const glm::mat4& view, const glm::mat4& projection) {
        // Column vectors: clip = projection * view * world * position.
        const auto mvp = projection * view * this->camera.world;
        if (!invertible(view) || !invertible(projection) || !invertible(mvp)) {
            return foundation::Error(sub_system::CameraError::InvalidMatrix,
                "Camera matrices must be finite and invertible");
        }
        this->camera.view = view;
        this->camera.projection = projection;
        this->camera.mvp = mvp;
        return {};
    }

    void CameraController::reset() {
        this->camera = types::make_default_model_camera();
    }
} // namespace enishi::core
