#pragma once
#include <array>
#include <component/animation_component.h>
#include <component/model_component.h>
#include <engine_types/collider/ray.h>
#include <optional>

namespace enishi::platform {
    class IWindow;
}

namespace enishi::core {
    struct CursorRay {
        types::Ray ray;
        glm::vec3 plane_normal;
    };

    [[nodiscard]] std::optional<CursorRay> read_cursor_ray(const platform::IWindow& window);

    // A temporary local-pose layer. Restore after skinning captures the pose, so an idle
    // animation or a stopped clip never accumulates last frame's cursor correction.
    class CursorPose {
      private:
        enum Joint : std::size_t { Neck, Head, LeftArm, LeftElbow, RightArm, RightElbow, Count };
        std::array<types::BoneIndex, Count> joints;
        std::array<types::BoneIndex, 2> wrists;
        std::array<glm::quat, Count> offsets;
        std::array<glm::quat, Count> originals;
        std::vector<glm::mat4> globals;
        std::vector<glm::mat4> external;
        std::vector<types::BoneIndex> traversal;
        std::optional<std::size_t> active_arm;
        bool applied = false;

      public:
        explicit CursorPose(const component::ModelComponent& model);
        void apply(const component::ModelComponent& model,
            component::AnimationComponent& animation,
            const std::optional<CursorRay>& cursor,
            float seconds);
        void restore(component::AnimationComponent& animation);

      private:
        void update_globals(
            const component::ModelComponent& model, const component::AnimationComponent& animation);
        [[nodiscard]] std::optional<glm::vec3> target(const CursorRay& cursor) const;
        void look(component::AnimationComponent& animation,
            Joint joint,
            const glm::vec3& target,
            float yaw_limit,
            float pitch_limit);
        void reach(const component::ModelComponent& model,
            component::AnimationComponent& animation,
            const glm::vec3& target);
        void aim(component::AnimationComponent& animation,
            Joint joint,
            const glm::vec3& from,
            const glm::vec3& to,
            float limit);
    };
} // namespace enishi::core
