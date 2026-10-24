#pragma once
#include "../../transition/pose_transition.h"
#include "cursor_pose_config.h"
#include <array>
#include <cstdint>
#include <engine_types/assets/model/addons/bone.h>
#include <engine_types/collider/obb.h>
#include <engine_types/collider/ray.h>
#include <optional>
#include <utility>
#include <vector>

namespace enishi::animation {
    struct CursorRay {
        types::Ray ray;
        glm::vec3 plane_normal;
    };

    struct CursorCapsule {
        glm::vec3 start;
        glm::vec3 end;
        float radius;
    };

    struct CursorArmFrame {
        std::vector<CursorCapsule> obstacles;
        glm::vec3 center;
        glm::vec3 outward;
        glm::vec3 up;
        glm::vec3 forward;
        float length;
        float arm_margin;
        float hand_margin;
    };

    struct CursorCollisionBone {
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

    struct CursorCollisionQuery {
        std::vector<std::size_t> limbs;
        std::size_t obstacle;
        bool forearm;
        bool hand;
    };

    struct CursorTransitionState {
        enum class Stage { Cursor, Jump, Hanging, File };
        PoseTransition transition;
        std::vector<component::AnimationBuffer> display;
        std::uint64_t revision = 0;
        Stage stage = Stage::Cursor;
        bool frame = false;
        bool initializing = false;
    };

    struct CursorIdleState {
        std::vector<component::AnimationBuffer> originals;
        types::BoneIndex torso = types::INVALID_BONE_INDEX;
        float time = 0;
        float weight = 0;
        bool applied = false;
    };

    struct CursorHangingState {
        std::vector<component::AnimationBuffer> display_pose;
        std::vector<component::AnimationBuffer> pose;
        std::vector<component::AnimationBuffer> originals;
        std::size_t side = 0;
        glm::vec3 return_offset{};
        glm::vec3 display_center{};
        glm::vec3 anchor{};
        glm::vec3 normal{0, 0, 1};
        glm::vec3 target{};
        glm::vec3 lag{};
        float weight = 0;
        float entry_time = 0;
        float landing_time = 0;
        float return_cycles = 1;
        float min_height = 0;
        bool active = false;
        bool landing = false;
    };

    struct CursorLocomotionState {
        std::vector<types::BoneIndex> roots;
        std::vector<glm::vec3> original_root_positions;
        std::vector<types::BoneIndex> leg_ik_goals;
        std::vector<std::pair<types::BoneIndex, component::AnimationBuffer>> leg_originals;
        std::array<std::array<types::BoneIndex, 3>, 2> legs;
        float wait = 0;
        float time = 0;
        float height = 0;
        float rest = 0;
    };

    struct CursorTransformCache {
        std::vector<glm::mat4> globals;
        std::vector<glm::mat4> external;
        std::vector<component::AnimationBuffer> cached_local;
        std::vector<glm::mat4> cached_external;
        std::vector<bool> changed_globals;
        std::vector<types::BoneIndex> traversal;
        bool valid = false;
    };

    struct CursorCollisionState {
        mutable std::vector<CursorCollisionBone> bones;
        std::array<std::vector<CursorCollisionQuery>, 2> queries;
        std::array<std::vector<std::size_t>, 2> hand_bounds;
        std::vector<glm::mat4> physics_globals;
    };

    struct CursorPoseState {
        enum Joint : std::size_t { Neck, Head, LeftArm, LeftElbow, RightArm, RightElbow, Count };
        enum BodyBone : std::size_t {
            BodyLeft,
            BodyRight,
            BodyNeck,
            BodyHead,
            BodyHips,
            BodyCount
        };
        CursorTransitionState transition_state;
        CursorIdleState idle;
        CursorHangingState hanging_state;
        CursorLocomotionState locomotion;
        CursorTransformCache transforms;
        CursorCollisionState collision;
        std::array<std::vector<types::BoneIndex>, 2> hand_reach_bones;
        std::array<types::BoneIndex, Count> joints;
        std::array<types::BoneIndex, 2> wrists;
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
    };
} // namespace enishi::animation
