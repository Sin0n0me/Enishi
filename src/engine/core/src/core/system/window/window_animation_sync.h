#pragma once
#include <core/system/interface_system.h>
#include <ecs/registry.h>
#include <optional>
#include <platform/window/interface_window.h>
#include <sub_system/camera/interface_camera_controller.h>
#include <sub_system/window/interface_window_animation_sync.h>

namespace enishi::core {
    class WindowAnimationSync final : public ISystem,
                                      public sub_system::IWindowAnimationSync,
                                      public sub_system::ICameraController {
      private:
        struct Target {
            types::HandleId model;
            types::BoneIndex bone;
            sub_system::WindowAnimationMode mode;
        };
        std::shared_ptr<ecs::Registry> registry;
        std::shared_ptr<platform::IWindow> window;
        std::shared_ptr<sub_system::ICameraController> camera;
        std::optional<Target> target;
        std::optional<glm::vec3> previous;
        glm::dvec2 remainder{0};
        glm::dvec2 compensation{0};
        [[nodiscard]] glm::mat4 screen_shift() const;
        [[nodiscard]] std::optional<glm::vec3> final_position() const;
        void synchronize(const glm::vec3& position);

      public:
        WindowAnimationSync(std::shared_ptr<ecs::Registry> registry,
            std::shared_ptr<platform::IWindow> window,
            std::shared_ptr<sub_system::ICameraController> camera);
        foundation::VoidResult<sub_system::WindowAnimationError> follow(types::HandleId model,
            types::BoneIndex bone,
            sub_system::WindowAnimationMode mode) override;
        void stop() override;
        types::UniformCamera get_camera() const override;
        foundation::VoidResult<sub_system::CameraError> set_view_projection(
            const glm::mat4& view, const glm::mat4& projection) override;
        void reset() override;
        void pre_update() override;
        void update(const types::DeltaTime& delta_time) override;
        void post_update() override;
        void render() const override;
    };
} // namespace enishi::core
