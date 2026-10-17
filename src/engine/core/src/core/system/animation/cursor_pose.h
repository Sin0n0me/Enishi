#pragma once
#include "cursor_pose_config.h"
#include "pose_transition.h"
#include <array>
#include <component/animation_component.h>
#include <component/model_component.h>
#include <engine_types/collider/obb.h>
#include <engine_types/collider/ray.h>
#include <optional>
#include <utility>

namespace enishi::platform {
    class IWindow;
}

namespace enishi::core {
    struct CursorRay {
        types::Ray ray;
        glm::vec3 plane_normal;
    };

    [[nodiscard]] std::optional<CursorRay> read_cursor_ray(const platform::IWindow& window,
        const types::UniformCamera& camera,
        bool allow_outside = false);

    // A temporary local-pose layer. Restore after skinning captures the pose, so an idle
    // animation or a stopped clip never accumulates last frame's cursor correction.
    class CursorPose {
      private:
        enum class PoseStage { Cursor, Jump, Hanging, File };
        static constexpr float HANGING_DROP_SECONDS = 0.3f;
        static constexpr float HANGING_LANDING_SECONDS = 0.3f;
        PoseTransition transition;
        std::vector<component::AnimationBuffer> transition_display;
        PoseStage transition_stage = PoseStage::Cursor;
        std::uint64_t transition_revision = 0;
        bool transition_frame = false;
        bool transition_initializing = false;
        [[nodiscard]] PoseStage pose_stage(const component::AnimationComponent& animation) const;
        void remember_transition_pose(const component::AnimationComponent& animation);
        void apply_idle_legs(const component::ModelComponent& model,
            component::AnimationComponent& animation,
            float sway);
        void apply_idle_arms(const component::ModelComponent& model,
            component::AnimationComponent& animation,
            float weight);
        enum Joint : std::size_t { Neck, Head, LeftArm, LeftElbow, RightArm, RightElbow, Count };
        std::array<types::BoneIndex, Count> joints;
        std::array<types::BoneIndex, 2> wrists;
        std::array<std::vector<types::BoneIndex>, 2> hand_reach_bones;
        std::optional<types::Ray> reach_ray;
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
        std::vector<component::AnimationBuffer> cached_local;
        std::vector<glm::mat4> cached_external;
        std::vector<bool> changed_globals;
        bool globals_cached = false;
        std::vector<types::BoneIndex> traversal;
        std::array<bool, 2> active_arms{};
        bool applied = false;
        float idle_time = 0;
        float idle_weight = 0;
        bool idle_applied = false;
        types::BoneIndex idle_torso = types::INVALID_BONE_INDEX;
        std::vector<component::AnimationBuffer> idle_originals;
        bool hanging = false;
        float hanging_weight = 0;
        float hanging_entry_time = 0;
        bool hanging_landing = false;
        float hanging_landing_time = 0;
        glm::vec3 hanging_return_offset{};
        glm::vec3 hanging_display_center{};
        float hanging_return_cycles = 1;
        std::vector<component::AnimationBuffer> hanging_display_pose;
        float hanging_min_height = 0;
        std::size_t hanging_side = 0;
        glm::vec3 hanging_anchor{};
        glm::vec3 hanging_normal{0, 0, 1};
        glm::vec3 hanging_target{};
        glm::vec3 hanging_lag{};
        std::vector<component::AnimationBuffer> hanging_pose;
        std::vector<component::AnimationBuffer> hanging_originals;
        CursorPoseConfig config;
        std::vector<types::BoneIndex> roots;
        std::vector<glm::vec3> original_root_positions;
        float jump_wait = 0;
        float jump_time = 0;
        float jump_height = 0;
        float jump_rest = 0;
        std::array<std::array<types::BoneIndex, 3>, 2> legs;
        std::vector<types::BoneIndex> leg_ik_goals;
        std::vector<std::pair<types::BoneIndex, component::AnimationBuffer>> leg_originals;
        struct CollisionBone {
            types::BoneIndex bone;
            types::OBB local;
            std::optional<types::OBB> world;
            glm::mat4 transform{0};
            glm::vec3 lower{};
            glm::vec3 upper{};
            int arm = -1;
            bool distal = false;
            bool hand = false;
            bool physics = false;
        };
        mutable std::vector<CollisionBone> collision_bones;
        struct CollisionQuery {
            std::size_t obstacle;
            bool forearm;
            bool hand;
            std::vector<std::size_t> limbs;
        };
        std::array<std::vector<CollisionQuery>, 2> collision_queries;
        std::array<std::vector<std::size_t>, 2> hand_bounds;
        std::vector<glm::mat4> collision_physics_globals;

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
} // namespace enishi::core
