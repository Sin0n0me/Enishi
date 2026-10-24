#pragma once
#include "camera_zoom_config.h"
#include <core/system/interface_system.h>
#include <memory>
#include <platform/window/interface_window.h>
#include <sub_system/camera/interface_camera_controller.h>

namespace enishi::core {
    class CameraZoomSystem final : public ISystem {
      private:
        std::shared_ptr<platform::IWindow> window;
        std::shared_ptr<sub_system::ICameraController> camera;
        CameraZoomConfig config;

      public:
        CameraZoomSystem(std::shared_ptr<platform::IWindow> window,
            std::shared_ptr<sub_system::ICameraController> camera,
            CameraZoomConfig config);
        void pre_update() override;
        void update(const types::DeltaTime& delta_time) override;
        void post_update() override;
        void render() const override;
    };
} // namespace enishi::core
