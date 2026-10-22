#pragma once
#include "cursor_pose_config.h"
#include "cursor_pose_state.h"
#include <array>
#include <component/animation_component.h>
#include <component/model_component.h>
#include <engine_types/collider/obb.h>
#include <engine_types/collider/ray.h>
#include <optional>
#include <utility>

namespace enishi::animation {
    struct CursorRay {
        types::Ray ray;
        glm::vec3 plane_normal;
    };

    // A temporary local-pose layer. Restore after skinning captures the pose, so an idle
    // animation or a stopped clip never accumulates last frame's cursor correction.
    class CursorPose {
      private:
        using PoseStage = CursorTransitionState::Stage;
        static constexpr float HANGING_DROP_SECONDS = 0.3f;
        static constexpr float HANGING_LANDING_SECONDS = 0.3f;
        [[nodiscard]] PoseStage pose_stage(const component::AnimationComponent& animation) const;
        void remember_transition_pose(const component::AnimationComponent& animation);
        void apply_idle_legs(const component::ModelComponent& model,
            component::AnimationComponent& animation,
            float sway);
        void apply_idle_arms(const component::ModelComponent& model,
            component::AnimationComponent& animation,
            float weight);
        enum Joint : std::size_t { Neck, Head, LeftArm, LeftElbow, RightArm, RightElbow, Count };
        CursorTransitionState transition_state;
        CursorIdleState idle;
        CursorHangingState hanging_state;
        CursorLocomotionState locomotion;
        CursorTransformCache transforms;
        CursorCollisionState collision;
        std::array<std::vector<types::BoneIndex>, 2> hand_reach_bones;
        std::array<types::BoneIndex, Count> joints;
        std::array<types::BoneIndex, 2> wrists;
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
        std::array<types::BoneIndex, 2> eyes;
        std::optional<types::Ray> reach_ray;
        std::array<glm::quat, Count> offsets;
        std::array<glm::quat, Count> originals;
        std::array<glm::quat, Count> bind_rotations;
        std::array<glm::vec3, 2> hinge_axes;
        std::array<glm::vec3, 2> upper_axes;
        std::array<float, 2> bind_flex{};
        glm::mat3 body_bind_inverse{1};
        glm::vec3 head_local_up{0, 1, 0};
        CursorPoseConfig config;
        std::array<bool, 2> active_arms{};
        bool applied = false;
        using Capsule = CursorCapsule;
        using ArmFrame = CursorArmFrame;
        using CollisionBone = CursorCollisionBone;
        using CollisionQuery = CursorCollisionQuery;

      public:
        void apply_transitioned(const component::ModelComponent& model,
            component::AnimationComponent& animation,
            const std::optional<CursorRay>& cursor,
            float seconds,
            bool idle_enabled);
        explicit CursorPose(const component::ModelComponent& model, CursorPoseConfig config = {});
        void apply(const component::ModelComponent& model,
            component::AnimationComponent& animation,
            const std::optional<CursorRay>& cursor,
            float seconds,
            bool idle_enabled = false);
        void restore(component::AnimationComponent& animation);
        [[nodiscard]] std::optional<std::pair<std::size_t, float>> pick_hand(
            const component::AnimationComponent& animation, const CursorRay& cursor) const;
        void begin_hanging(std::size_t side,
            const component::AnimationComponent& animation,
            const CursorRay& cursor);
        void end_hanging();
        [[nodiscard]] bool is_hanging() const;
        [[nodiscard]] bool suppress_hanging_ik(types::BoneIndex bone) const;

      private:
        void apply_idle(const component::ModelComponent& model,
            component::AnimationComponent& animation,
            bool enabled,
            float seconds);
        void blend_hanging_pose(component::AnimationComponent& animation) const;
        bool apply_hanging_landing(const component::ModelComponent& model,
            component::AnimationComponent& animation,
            float seconds);
        void apply_hanging_walk(const component::ModelComponent& model,
            component::AnimationComponent& animation,
            float seconds);
        void translate_hanging_roots(
            component::AnimationComponent& animation, const glm::vec3& offset) const;
        bool apply_hanging(const component::ModelComponent& model,
            component::AnimationComponent& animation,
            const std::optional<CursorRay>& cursor,
            float seconds);
        void update_hanging_ik_targets(
            const component::ModelComponent& model, component::AnimationComponent& animation);
        void prepare_hanging_limbs(
            const component::ModelComponent& model, component::AnimationComponent& animation);
        void hang_limb(const component::ModelComponent& model,
            component::AnimationComponent& animation,
            types::BoneIndex start,
            types::BoneIndex end,
            const glm::vec3& target_direction);
        void initialize_legs(const component::ModelComponent& model);
        void crouch(const component::ModelComponent& model,
            component::AnimationComponent& animation,
            float weight);
        void bend_leg(const component::ModelComponent& model,
            component::AnimationComponent& animation,
            std::size_t side,
            const glm::mat4& ankle);
        void apply_jump(const component::ModelComponent& model,
            component::AnimationComponent& animation,
            const std::optional<glm::vec3>& point,
            float seconds);
        [[nodiscard]] float desired_jump_height(const glm::vec3& point) const;
        [[nodiscard]] bool cursor_in_arm_reach(std::size_t side) const;
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
        void initialize_collision(const component::ModelComponent& model);
        void initialize_collision_queries(const component::ModelComponent& model);
        [[nodiscard]] const CollisionBone& collision_bound(std::size_t index) const;
        [[nodiscard]] bool mesh_arm_clear(std::size_t side) const;
        [[nodiscard]] float hand_radius(std::size_t side) const;
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
} // namespace enishi::animation
