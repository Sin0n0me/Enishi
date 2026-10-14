#pragma once
#include "cursor_pose_config.h"
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
        std::array<types::BoneIndex, 2> eyes;
        std::array<glm::quat, Count> offsets;
        std::array<glm::quat, Count> originals;
        std::array<glm::quat, Count> bind_rotations;
        std::array<glm::vec3, 2> hinge_axes;
        std::array<glm::vec3, 2> upper_axes;
        std::array<float, 2> bind_flex{};
        enum BodyBone : std::size_t {
            BodyLeft,
            BodyRight,
            BodyNeck,
            BodyHead,
            BodyHips,
            BodyCount
        };
        std::array<types::BoneIndex, BodyCount> body_bones;
        types::BoneIndex body_anchor = types::INVALID_BONE_INDEX;
        glm::mat3 body_bind_inverse{1};
        glm::vec3 head_local_up{0, 1, 0};
        struct Capsule {
            glm::vec3 start;
            glm::vec3 end;
            float radius;
        };
        struct ArmFrame {
            glm::vec3 center;
            glm::vec3 outward;
            glm::vec3 up;
            glm::vec3 forward;
            float length;
            float arm_margin;
            float hand_margin;
            std::vector<Capsule> obstacles;
        };
        std::vector<glm::mat4> globals;
        std::vector<glm::mat4> external;
        std::vector<types::BoneIndex> traversal;
        std::array<bool, 2> active_arms{};
        bool applied = false;
        CursorPoseConfig config;

      public:
        explicit CursorPose(const component::ModelComponent& model, CursorPoseConfig config = {});
        void apply(const component::ModelComponent& model,
            component::AnimationComponent& animation,
            const std::optional<CursorRay>& cursor,
            float seconds);
        void restore(component::AnimationComponent& animation);

      private:
        void update_globals(
            const component::ModelComponent& model, const component::AnimationComponent& animation);
        [[nodiscard]] std::optional<glm::vec3> target(const CursorRay& cursor) const;
        [[nodiscard]] std::optional<glm::vec3> gaze_offset(const CursorRay& cursor) const;
        void look(component::AnimationComponent& animation,
            Joint joint,
            const glm::vec3& offset,
            float yaw_limit,
            float pitch_limit);
        void reach(const component::ModelComponent& model,
            component::AnimationComponent& animation,
            const glm::vec3& target);
        void initialize_arms(const component::ModelComponent& model);
        [[nodiscard]] bool can_reach(std::size_t side, const glm::vec3& target) const;
        [[nodiscard]] ArmFrame arm_frame(std::size_t side) const;
        [[nodiscard]] bool arm_clear(std::size_t side, const ArmFrame& frame) const;
        void constrain_arm(component::AnimationComponent& animation, std::size_t side) const;
        bool reach_side(const component::ModelComponent& model,
            component::AnimationComponent& animation,
            std::size_t side,
            const glm::vec3& target);
        void solve_arm(const component::ModelComponent& model,
            component::AnimationComponent& animation,
            std::size_t side,
            const glm::vec3& target,
            const ArmFrame& frame);
        void protect_arms(
            const component::ModelComponent& model, component::AnimationComponent& animation);
        void limit_arm_speed(const component::ModelComponent& model,
            component::AnimationComponent& animation,
            const std::array<glm::quat, Count>& previous,
            float seconds);
    };
} // namespace enishi::core
