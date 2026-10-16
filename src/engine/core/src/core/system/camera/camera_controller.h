#pragma once
#include <sub_system/camera/interface_camera_controller.h>

namespace enishi::core {
    class CameraController final : public sub_system::ICameraController {
      private:
        types::UniformCamera camera;

      public:
        CameraController();
        [[nodiscard]] types::UniformCamera get_camera() const override;
        [[nodiscard]] foundation::VoidResult<sub_system::CameraError> set_view_projection(
            const glm::mat4& view, const glm::mat4& projection) override;
        void reset() override;
    };
} // namespace enishi::core
