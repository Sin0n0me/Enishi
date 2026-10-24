#pragma once
#include <collider/collider.h>
#include <core/system/interface_system.h>
#include <debug/overlay/interface_debug_overlay.h>
#include <ecs/registry.h>
#include <memory>
#include <platform/window/interface_window.h>
#include <sub_system/camera/interface_camera_controller.h>

namespace enishi::core {
    class OBBDebugSystem final : public ISystem {
      private:
        std::shared_ptr<ecs::Registry> registry;
        std::shared_ptr<collider::Collider> collider;
        std::shared_ptr<platform::IWindow> window;
        std::shared_ptr<sub_system::ICameraController> camera;
        std::shared_ptr<debug::IDebugOverlay> overlay;
        bool enabled = false;

      public:
        OBBDebugSystem(std::shared_ptr<ecs::Registry> registry,
            std::shared_ptr<collider::Collider> collider,
            std::shared_ptr<platform::IWindow> window,
            std::shared_ptr<sub_system::ICameraController> camera,
            std::shared_ptr<debug::IDebugOverlay> overlay);
        void pre_update(void) override;
        void update(const types::DeltaTime& delta_time) override;
        void post_update(void) override;
        void render(void) const override;
    };
} // namespace enishi::core
