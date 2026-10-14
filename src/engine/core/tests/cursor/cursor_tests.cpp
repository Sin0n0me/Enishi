#include <core/system/animation/cursor_pose.h>
#include <core/system/skinning/skinning_system.h>
#include <cstdlib>
#include <engine_types/renderer/uniform_buffer/camera.h>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <sstream>

using namespace enishi;

namespace {
    constexpr float TOLERANCE = 0.001f;
    constexpr float SETTLE_SECONDS = 3.0f;
    constexpr glm::quat IDENTITY(1, 0, 0, 0);
    enum Bone : std::size_t {
        Root,
        Neck,
        Head,
        Arm,
        Twist,
        Elbow,
        Wrist,
        OtherArm,
        OtherElbow,
        OtherWrist,
        Count
    };

    void check(bool condition, const char* message) {
        if (!condition) {
            std::cerr << message << '\n';
            std::exit(EXIT_FAILURE);
        }
    }

    glm::mat4 global(const component::ModelComponent& model,
        const component::AnimationComponent& animation,
        types::BoneIndex index) {
        const auto& pose = animation.animation[index];
        const auto local = glm::translate(glm::mat4(1), pose.position) *
                           glm::mat4_cast(pose.rotation) * glm::scale(glm::mat4(1), pose.scale);
        const auto parent = model.bone_node[index].parent;
        return parent < model.bone_node.size() ? global(model, animation, parent) * local : local;
    }

    struct Fixture {
        component::ModelComponent model;
        component::AnimationComponent animation;
        Fixture();
    };

    Fixture::Fixture() {
        this->model.bone_names = {
            "root", "首", "頭", "左腕", "twist", "左ひじ", "左手首", "右腕", "右ひじ", "右手首"};
        this->model.bone_node.resize(Count);
        this->model.bind_bone.resize(Count);
        this->animation.animation.resize(Count);
        this->animation.global.resize(Count, glm::mat4(1));
        const std::array<types::BoneIndex, Count> parents{types::INVALID_BONE_INDEX,
            Root,
            Neck,
            Root,
            Arm,
            Twist,
            Elbow,
            Root,
            OtherArm,
            OtherElbow};
        const std::array<glm::vec3, Count> positions{glm::vec3(0),
            glm::vec3(0, 8, 0),
            glm::vec3(0, 1, 0),
            glm::vec3(1, 7, 0),
            glm::vec3(1, 0, 0),
            glm::vec3(1, 0, 0),
            glm::vec3(2, 0, 0),
            glm::vec3(-1, 7, 0),
            glm::vec3(-2, 0, 0),
            glm::vec3(-2, 0, 0)};
        for (std::size_t index = 0; index < Count; ++index) {
            this->model.bone_node[index].parent = parents[index];
            if (parents[index] < Count) {
                this->model.bone_node[parents[index]].children.push_back(index);
            }
            this->animation.animation[index].position = positions[index];
            this->model.bind_bone[index].local = glm::translate(glm::mat4(1), positions[index]);
        }
        for (std::size_t index = 0; index < Count; ++index) {
            const auto matrix = global(this->model, this->animation, index);
            this->model.bind_bone[index].global = matrix;
            this->model.bind_bone[index].global_inverse = glm::inverse(matrix);
            this->animation.global[index] = matrix;
        }
    }

    core::CursorRay cursor(float x, float y) {
        return {{{x, y, -20}, {0, 0, 1}}, {0, 0, 1}};
    }

    void look_and_reach() {
        Fixture fixture;
        core::CursorPose pose(fixture.model);
        const auto before = fixture.animation.animation;
        const glm::vec3 target(3, 7, -1.08f);
        const auto original_hand = glm::vec3(global(fixture.model, fixture.animation, Wrist)[3]);
        pose.apply(fixture.model, fixture.animation, cursor(target.x, target.y), SETTLE_SECONDS);
        const auto head = global(fixture.model, fixture.animation, Head);
        const auto facing = glm::normalize(glm::mat3(head) * glm::vec3(0, 0, -1));
        check(facing.x > 0 && facing.y < 0, "Head must turn toward the cursor");
        const auto shoulder = glm::vec3(global(fixture.model, fixture.animation, Arm)[3]);
        const auto elbow = glm::vec3(global(fixture.model, fixture.animation, Elbow)[3]);
        const auto wrist = glm::vec3(global(fixture.model, fixture.animation, Wrist)[3]);
        check(glm::length(wrist - target) < glm::length(original_hand - target),
            "Hand must approach the cursor");
        check(glm::length(wrist - target) < TOLERANCE,
            "Reachable target must be reached through an intermediate twist bone");
        check(std::abs(glm::length(elbow - shoulder) - 2.0f) < TOLERANCE &&
                  std::abs(glm::length(wrist - elbow) - 2.0f) < TOLERANCE,
            "Arm lengths must not change");
        check(fixture.animation.animation[OtherArm].rotation == IDENTITY,
            "Only the near arm should reach");
        const auto neck_forward = fixture.animation.animation[Neck].rotation * glm::vec3(0, 0, -1);
        check(std::abs(std::atan2(-neck_forward.x, -neck_forward.z)) <
                  glm::radians(core::CursorPoseConfig{}.neck_yaw) + TOLERANCE,
            "Neck yaw must stay within its limit");
        check(fixture.animation.animation[Twist].rotation == IDENTITY,
            "Do not write intermediate or unrelated bones");
        pose.restore(fixture.animation);
        for (std::size_t index = 0; index < Count; ++index) {
            check(fixture.animation.animation[index].rotation == before[index].rotation &&
                      fixture.animation.animation[index].position == before[index].position,
                "Source pose must be restored exactly");
        }
    }

    void smoothing_and_return() {
        Fixture fine;
        Fixture coarse;
        core::CursorPose fine_pose(fine.model);
        core::CursorPose coarse_pose(coarse.model);
        for (int frame = 0; frame < 60; ++frame) {
            fine_pose.apply(fine.model, fine.animation, cursor(3, 8), 1.0f / 60.0f);
            if (frame + 1 < 60) {
                fine_pose.restore(fine.animation);
            }
        }
        coarse_pose.apply(coarse.model, coarse.animation, cursor(3, 8), 1.0f);
        check(glm::length(fine.animation.animation[Head].rotation -
                          coarse.animation.animation[Head].rotation) < TOLERANCE,
            "Smoothing must be frame-rate independent and must not accumulate corrections");
        fine_pose.restore(fine.animation);
        fine_pose.apply(fine.model, fine.animation, std::nullopt, 0.016f);
        check(glm::length(fine.animation.animation[Head].rotation - IDENTITY) > 0.01f,
            "Leaving the window should fade rather than snap");
        fine_pose.restore(fine.animation);
        fine_pose.apply(fine.model, fine.animation, cursor(100, 100), SETTLE_SECONDS);
        check(glm::length(fine.animation.animation[Head].rotation - IDENTITY) < TOLERANCE &&
                  glm::length(fine.animation.animation[Arm].rotation - IDENTITY) < TOLERANCE,
            "A distant cursor must restore the original pose");
    }

    void limits_and_missing_bones() {
        Fixture fixture;
        core::CursorPose pose(fixture.model);
        pose.apply(fixture.model, fixture.animation, cursor(6, 8), SETTLE_SECONDS);
        const auto shoulder = glm::vec3(global(fixture.model, fixture.animation, Arm)[3]);
        const auto wrist = glm::vec3(global(fixture.model, fixture.animation, Wrist)[3]);
        check(glm::length(wrist - shoulder) < 4.0f + TOLERANCE,
            "Unreachable targets must not stretch the arm");
        check(fixture.animation.animation[Arm].rotation == IDENTITY,
            "An out-of-range arm must remain in its authored pose");
        pose.restore(fixture.animation);
        fixture.model.bone_names[Elbow] = "unrecognized";
        fixture.model.bone_names[Neck] = "unrecognized neck";
        types::PhysicsRigidBody body{};
        body.relate_bone_index = Head;
        body.kind = types::RigidBodyKind::Dynamic;
        fixture.model.rigid_bodies.push_back(body);
        core::CursorPose unsupported(fixture.model);
        unsupported.apply(fixture.model, fixture.animation, cursor(3, 8), SETTLE_SECONDS);
        check(fixture.animation.animation[Head].rotation == IDENTITY,
            "Physics-owned head must be untouched");
        check(fixture.animation.animation[Arm].rotation == IDENTITY,
            "Incomplete chains must be skipped");
        unsupported.restore(fixture.animation);
        unsupported.apply(fixture.model, fixture.animation, cursor(0, 7), SETTLE_SECONDS);
        check(
            fixture.animation.animation[Arm].rotation == IDENTITY &&
                glm::length(fixture.animation.animation[OtherArm].rotation - IDENTITY) > TOLERANCE,
            "A missing arm chain must not prevent the valid arm from tracking the center");
        unsupported.restore(fixture.animation);
        Fixture zero;
        zero.animation.animation[Wrist].position = glm::vec3(0);
        core::CursorPose degenerate(zero.model);
        degenerate.apply(zero.model, zero.animation, cursor(3, 8), SETTLE_SECONDS);
        for (const auto& bone : zero.animation.animation) {
            check(std::isfinite(bone.rotation.w), "Zero-length links must not produce NaNs");
        }
    }

    void animated_pose() {
        Fixture fixture;
        fixture.animation.animation[Root].position = glm::vec3(2, 0, 0);
        fixture.animation.animation[Root].rotation =
            glm::angleAxis(glm::radians(15.0f), glm::vec3(0, 1, 0));
        fixture.animation.animation[Arm].rotation =
            glm::angleAxis(glm::radians(-25.0f), glm::vec3(0, 0, 1));
        const auto before = fixture.animation.animation;
        core::CursorPose pose(fixture.model);
        pose.apply(fixture.model, fixture.animation, cursor(5, 7), SETTLE_SECONDS);
        check(fixture.animation.animation[Root].rotation == before[Root].rotation,
            "Animated root must be preserved");
        check(glm::length(fixture.animation.animation[Arm].rotation - before[Arm].rotation) >
                  TOLERANCE,
            "Cursor pose must layer over an animated arm");
        pose.restore(fixture.animation);
        check(fixture.animation.animation[Arm].rotation == before[Arm].rotation,
            "Restore must retain the clip's rotation, not reset to bind pose");
    }

    void skinning_and_physics_pose() {
        Fixture fixture;
        auto registry = std::make_shared<ecs::Registry>();
        const auto entity = registry->create();
        check(registry->emplace<component::ModelComponent>(entity, fixture.model).is_ok(),
            "Insert model");
        check(registry->emplace<component::AnimationComponent>(entity, fixture.animation).is_ok(),
            "Insert animation");
        check(registry->emplace<component::SkinningComponent>(entity).is_ok(), "Insert skinning");
        component::IKComponent ik;
        ik.rotation.resize(Count, IDENTITY);
        ik.globals = fixture.animation.global;
        check(registry->emplace<component::IKComponent>(entity, ik).is_ok(), "Insert IK");
        component::PhysicsComponent physics;
        physics.global = fixture.animation.global;
        physics.local.resize(Count, glm::mat4(1));
        check(registry->emplace<component::PhysicsComponent>(entity, physics).is_ok(),
            "Insert physics pose");
        auto& animation = registry->get<component::AnimationComponent>(entity).unwrap_mut();
        auto& model = registry->get<component::ModelComponent>(entity).unwrap_mut();
        core::CursorPose pose(model);
        core::SkinningSystem skinning(registry, nullptr);
        pose.apply(model, animation, cursor(3, 7), SETTLE_SECONDS);
        const auto expected = global(model, animation, Wrist);
        skinning.update(types::DeltaTime(0.016f));
        pose.restore(animation);
        const auto& matrices =
            registry->get<component::SkinningComponent>(entity).unwrap().skinning_matrices;
        const auto result = matrices[Wrist] * model.bind_bone[Wrist].global;
        const auto& physics_result =
            registry->get<component::PhysicsComponent>(entity).unwrap().global[Wrist];
        check(glm::length(glm::vec3(result[3] - expected[3])) < TOLERANCE,
            "Render pose must retain cursor IK after the source pose is restored");
        check(glm::length(glm::vec3(physics_result[3] - expected[3])) < TOLERANCE,
            "Physics must receive the corrected pose before simulation");
        check(animation.animation[Arm].rotation == IDENTITY, "Source pose must remain reusable");
        model.visible = false;
        const auto hidden_matrix = matrices[Wrist];
        animation.animation[Root].position = glm::vec3(100);
        skinning.update(types::DeltaTime(0.016f));
        check(matrices[Wrist] == hidden_matrix, "Hidden model must not be updated");
    }

    float distance_to_axis(const glm::vec3& point, const glm::vec3& start, const glm::vec3& end) {
        const auto axis = end - start;
        // Closest point on the segment: start + clamp(dot(p-start,axis)/|axis|²)*axis.
        const float along =
            glm::clamp(glm::dot(point - start, axis) / glm::dot(axis, axis), 0.0f, 1.0f);
        return glm::length(point - start - along * axis);
    }

    void check_arm_safety(const Fixture& fixture) {
        constexpr float BODY_RADIUS = 0.64f;
        constexpr float LIMB_CLEARANCE = 0.15f;
        constexpr float HAND_CLEARANCE = 0.8f;
        constexpr int SEGMENT_SAMPLES = 40;
        const auto root = global(fixture.model, fixture.animation, Root);
        const auto up = glm::normalize(glm::mat3(root) * glm::vec3(0, 1, 0));
        const auto chest = glm::vec3(root * glm::vec4(0, 7, 0, 1));
        const auto pelvis = chest - up * 3.2f;
        const auto neck = glm::vec3(global(fixture.model, fixture.animation, Neck)[3]);
        const auto head = glm::vec3(global(fixture.model, fixture.animation, Head)[3]);
        const auto head_up = glm::normalize(
            glm::mat3(global(fixture.model, fixture.animation, Head)) * glm::vec3(0, 1, 0));
        const auto head_top = head + head_up * 1.1f;
        for (std::size_t side = 0; side < 2; ++side) {
            const auto arm = side == 0 ? Arm : OtherArm;
            const auto elbow = side == 0 ? Elbow : OtherElbow;
            const auto wrist = side == 0 ? Wrist : OtherWrist;
            const auto shoulder_position =
                glm::vec3(global(fixture.model, fixture.animation, arm)[3]);
            const auto elbow_position =
                glm::vec3(global(fixture.model, fixture.animation, elbow)[3]);
            const auto wrist_position =
                glm::vec3(global(fixture.model, fixture.animation, wrist)[3]);
            const auto rotation = fixture.animation.animation[elbow].rotation;
            const float sign = side == 0 ? 1.0f : -1.0f;
            const float flex = 2.0f * std::atan2(rotation.y * sign, rotation.w);
            check(std::abs(rotation.x) < TOLERANCE && std::abs(rotation.z) < TOLERANCE,
                "Elbow must remain a single anatomical hinge, including transition frames");
            check(flex > -TOLERANCE && flex < glm::radians(135.0f) + TOLERANCE,
                "Elbow must not hyperextend or fold beyond its limit");
            const auto shoulder_rotation = fixture.animation.animation[arm].rotation;
            const auto shoulder_axis = glm::vec3(sign, 0, 0);
            const float twist = 2.0f * std::atan2(shoulder_rotation.x * sign, shoulder_rotation.w);
            const auto swing =
                shoulder_rotation * glm::inverse(glm::angleAxis(twist, shoulder_axis));
            const float swing_angle = 2.0f * std::acos(glm::clamp(std::abs(swing.w), 0.0f, 1.0f));
            check(std::abs(twist) < glm::radians(80.0f) + TOLERANCE &&
                      swing_angle < glm::radians(100.0f) + TOLERANCE,
                "Shoulder twist and swing must remain within their anatomical limits");
            check(std::abs(glm::length(elbow_position - shoulder_position) - 2.0f) < TOLERANCE &&
                      std::abs(glm::length(wrist_position - elbow_position) - 2.0f) < TOLERANCE,
                "Safety correction must preserve arm lengths");
            for (int sample = 0; sample < SEGMENT_SAMPLES + 1; ++sample) {
                const float t = static_cast<float>(sample) / static_cast<float>(SEGMENT_SAMPLES);
                for (const auto point : {glm::mix(shoulder_position, elbow_position, t),
                         glm::mix(elbow_position, wrist_position, t)}) {
                    check(distance_to_axis(point, pelvis, neck) > BODY_RADIUS + LIMB_CLEARANCE,
                        "The complete upper arm and forearm must clear the torso");
                    check(distance_to_axis(point, head, head_top) > BODY_RADIUS + LIMB_CLEARANCE,
                        "The complete upper arm and forearm must clear the head");
                }
            }
            check(distance_to_axis(wrist_position, pelvis, neck) > BODY_RADIUS + HAND_CLEARANCE,
                "Leave enough space for the hand, not only the wrist, near the chest");
            check(distance_to_axis(wrist_position, head, head_top) > BODY_RADIUS + HAND_CLEARANCE,
                "Leave enough space for the hand near the face");
        }
    }

    void body_avoidance_and_hinge() {
        constexpr int FRAMES = 180;
        constexpr float FRAME_SECONDS = 1.0f / 60.0f;
        Fixture fixture;
        core::CursorPose pose(fixture.model);
        int moving_frames = 0;
        for (int frame = 0; frame < FRAMES; ++frame) {
            // Cross the chest, face and centerline repeatedly, forcing arm switches and blending.
            const float phase = static_cast<float>(frame) * 0.08f;
            pose.apply(fixture.model,
                fixture.animation,
                cursor(2.5f * std::sin(phase), 7.0f + 2.8f * std::sin(phase * 0.7f)),
                FRAME_SECONDS);
            check_arm_safety(fixture);
            if (glm::length(fixture.animation.animation[Arm].rotation - IDENTITY) > TOLERANCE ||
                glm::length(fixture.animation.animation[OtherArm].rotation - IDENTITY) >
                    TOLERANCE) {
                ++moving_frames;
            }
            pose.restore(fixture.animation);
        }
        check(moving_frames > FRAMES / 2,
            "Avoidance must retain reaching instead of disabling both arms");
        for (int frame = 0; frame < FRAMES; ++frame) {
            pose.apply(fixture.model, fixture.animation, std::nullopt, FRAME_SECONDS);
            check_arm_safety(fixture);
            pose.restore(fixture.animation);
        }
    }

    void rotated_body_avoidance() {
        Fixture fixture;
        fixture.animation.animation[Root].rotation =
            glm::angleAxis(glm::radians(35.0f), glm::vec3(0, 0, 1)) *
            glm::angleAxis(glm::radians(25.0f), glm::vec3(0, 1, 0));
        core::CursorPose pose(fixture.model);
        for (const auto point : {glm::vec2(-3, 6), glm::vec2(-4, 8), glm::vec2(0, 6)}) {
            pose.apply(fixture.model, fixture.animation, cursor(point.x, point.y), SETTLE_SECONDS);
            check_arm_safety(fixture);
            pose.restore(fixture.animation);
        }
    }

    void release_to_authored_elbow() {
        Fixture fixture;
        const auto authored = glm::angleAxis(glm::radians(15.0f), glm::vec3(1, 0, 0));
        fixture.animation.animation[Elbow].rotation = authored;
        core::CursorPose pose(fixture.model);
        pose.apply(fixture.model, fixture.animation, cursor(3, 7), SETTLE_SECONDS);
        pose.restore(fixture.animation);
        constexpr int RETURN_FRAMES = 180;
        for (int frame = 0; frame < RETURN_FRAMES; ++frame) {
            pose.apply(fixture.model, fixture.animation, std::nullopt, 1.0f / 60.0f);
            if (frame + 1 < RETURN_FRAMES) {
                pose.restore(fixture.animation);
            }
        }
        check(glm::length(fixture.animation.animation[Elbow].rotation - authored) < TOLERANCE,
            "After release, procedural hinge limits must not keep modifying the authored clip");
    }
    void configurable_limits() {
        std::istringstream settings("\xEF\xBB\xBF# custom settings\nneck_yaw 4\nneck_pitch 2\n"
                                    "head_yaw 8\nhead_pitch 3\narm_speed 45 # degrees/sec\n");
        auto parsed = core::parse_cursor_pose_config(settings);
        check(parsed.is_ok(), "Valid cursor configuration must load");
        const auto config = std::move(parsed).unwrap();
        check(config.neck_yaw == 4 && config.neck_pitch == 2 && config.head_yaw == 8 &&
                  config.head_pitch == 3 && config.arm_speed == 45,
            "All configured values must be retained");
        for (const auto text : {"neck_yaw -1",
                 "neck_pitch 91",
                 "arm_speed 0",
                 "arm_speed 721",
                 "arm_speed nan",
                 "arm_speed inf",
                 "neck_yaw 2 extra",
                 "unknown 5",
                 "head_yaw",
                 "neck_yaw 2\nneck_yaw 3"}) {
            std::istringstream invalid(text);
            check(core::parse_cursor_pose_config(invalid).is_err(),
                "Invalid cursor configuration must be rejected");
        }
        std::istringstream empty;
        auto defaults = core::parse_cursor_pose_config(empty);
        check(defaults.is_ok() && defaults.unwrap().neck_yaw == 10,
            "Omitted settings must use stricter defaults");
        Fixture fixture;
        core::CursorPose pose(fixture.model, config);
        pose.apply(fixture.model, fixture.animation, cursor(5, 3), SETTLE_SECONDS);
        for (const auto bone : {Neck, Head}) {
            const auto direction = fixture.animation.animation[bone].rotation * glm::vec3(0, 0, -1);
            // qYaw*qPitch facing -Z yields yaw=atan2(-x,-z), pitch=asin(y).
            const float yaw = glm::degrees(std::atan2(-direction.x, -direction.z));
            const float pitch = glm::degrees(std::asin(direction.y));
            const float yaw_limit = bone == Neck ? config.neck_yaw : config.head_yaw;
            const float pitch_limit = bone == Neck ? config.neck_pitch : config.head_pitch;
            check(
                std::abs(yaw) < yaw_limit + TOLERANCE && std::abs(pitch) < pitch_limit + TOLERANCE,
                "Configured neck and head angle limits must be respected");
            check(std::abs(yaw) > TOLERANCE, "Configuration must not disable gaze");
        }
    }

    void bounded_reach_speed() {
        constexpr float SPEED = 45.0f;
        constexpr int FRAMES = 240;
        for (const float seconds : {1.0f / 30.0f, 1.0f / 60.0f, 1.0f / 120.0f}) {
            Fixture fixture;
            core::CursorPoseConfig config;
            config.arm_speed = SPEED;
            core::CursorPose pose(fixture.model, config);
            std::array<glm::quat, 4> previous{IDENTITY, IDENTITY, IDENTITY, IDENTITY};
            constexpr std::array<Bone, 4> ARMS{Arm, Elbow, OtherArm, OtherElbow};
            float total_motion = 0;
            for (int frame = 0; frame < FRAMES; ++frame) {
                // Enter/leave the boundary, switch sides, then move to the face and torso.
                const std::array<glm::vec2, 6> targets{glm::vec2(6.5f, 7),
                    glm::vec2(20, 7),
                    glm::vec2(-6.5f, 7),
                    glm::vec2(0, 9),
                    glm::vec2(0, 6),
                    glm::vec2(20, 7)};
                const auto target = targets[static_cast<std::size_t>(frame / 40)];
                pose.apply(fixture.model, fixture.animation, cursor(target.x, target.y), seconds);
                for (std::size_t joint = 0; joint < ARMS.size(); ++joint) {
                    const auto rotation = fixture.animation.animation[ARMS[joint]].rotation;
                    const auto delta = glm::inverse(previous[joint]) * rotation;
                    const float angle = glm::degrees(
                        2.0f * std::atan2(glm::length(glm::vec3(delta.x, delta.y, delta.z)),
                                   std::abs(delta.w)));
                    check(angle < SPEED * seconds + TOLERANCE,
                        "Reaching, switching and returning must respect configured angular speed");
                    total_motion += angle;
                    previous[joint] = rotation;
                }
                check_arm_safety(fixture);
                pose.restore(fixture.animation);
            }
            check(total_motion > SPEED, "Speed limiting must still allow arm movement");
        }
    }
    void overlapping_hand_tracking() {
        constexpr float START_X = 3.0f;
        constexpr float CROSS_CENTER_X = 0.75f;
        constexpr float TARGET_HEIGHT = 7.0f;
        for (const float side : {-1.0f, 1.0f}) {
            Fixture fixture;
            core::CursorPose pose(fixture.model);
            const auto first = side > 0 ? Arm : OtherArm;
            const auto other = side > 0 ? OtherArm : Arm;
            pose.apply(fixture.model,
                fixture.animation,
                cursor(side * START_X, TARGET_HEIGHT),
                SETTLE_SECONDS);
            pose.restore(fixture.animation);
            // Both hands should follow across the overlapping range, regardless of
            // which side was active before entering it.
            for (const float x : {-CROSS_CENTER_X, CROSS_CENTER_X, -CROSS_CENTER_X}) {
                pose.apply(fixture.model,
                    fixture.animation,
                    cursor(side * x, TARGET_HEIGHT),
                    SETTLE_SECONDS);
                check(
                    glm::length(fixture.animation.animation[first].rotation - IDENTITY) > TOLERANCE,
                    "The original hand must keep tracking beyond the body center");
                check(
                    glm::length(fixture.animation.animation[other].rotation - IDENTITY) > TOLERANCE,
                    "Both hands must track inside the overlapping range");
                check_arm_safety(fixture);
                pose.restore(fixture.animation);
            }
            pose.apply(fixture.model,
                fixture.animation,
                cursor(-side * START_X, TARGET_HEIGHT),
                SETTLE_SECONDS);
            check(glm::length(fixture.animation.animation[other].rotation - IDENTITY) > TOLERANCE,
                "The in-range hand must continue beyond the opposite shoulder");
            check(glm::length(fixture.animation.animation[first].rotation - IDENTITY) < TOLERANCE,
                "Only the out-of-range hand must return to its authored pose");
            check_arm_safety(fixture);
            pose.restore(fixture.animation);
            pose.apply(fixture.model, fixture.animation, cursor(0, TARGET_HEIGHT), SETTLE_SECONDS);
            for (const auto arm : {Arm, OtherArm}) {
                check(glm::length(fixture.animation.animation[arm].rotation - IDENTITY) > TOLERANCE,
                    "Both hands must resume tracking after returning to the center");
            }
            pose.restore(fixture.animation);
            pose.apply(fixture.model, fixture.animation, std::nullopt, SETTLE_SECONDS);
            for (const auto arm : {Arm, OtherArm}) {
                check(glm::length(fixture.animation.animation[arm].rotation - IDENTITY) < TOLERANCE,
                    "Both hands must release when the cursor leaves");
            }
        }
    }

    void face_center_gaze() {
        constexpr float FACE_OFFSET = 0.5f;
        constexpr float FACE_DEPTH = -0.25f;
        constexpr float CROSSING_STEP = 0.001f;
        constexpr float CONTINUITY_TOLERANCE = 0.005f;
        for (const bool with_eyes : {false, true}) {
            Fixture fixture;
            if (with_eyes) {
                for (const auto side : {-1.0f, 1.0f}) {
                    const auto index =
                        static_cast<types::BoneIndex>(fixture.model.bone_node.size());
                    fixture.model.bone_names.push_back(side < 0 ? "左目" : "右目");
                    fixture.model.bone_node.emplace_back();
                    fixture.model.bone_node.back().parent = Head;
                    fixture.model.bone_node[Head].children.push_back(index);
                    fixture.animation.animation.emplace_back();
                    fixture.animation.animation.back().position =
                        glm::vec3(side * FACE_OFFSET, FACE_OFFSET, FACE_DEPTH);
                    fixture.animation.global.push_back(
                        global(fixture.model, fixture.animation, index));
                    fixture.model.bind_bone.emplace_back();
                    auto& bind = fixture.model.bind_bone.back();
                    bind.local =
                        glm::translate(glm::mat4(1), fixture.animation.animation.back().position);
                    bind.global = fixture.animation.global.back();
                    bind.global_inverse = glm::inverse(bind.global);
                }
            }
            const glm::vec3 center(0, 9.0f + FACE_OFFSET, with_eyes ? FACE_DEPTH : 0.0f);
            const auto camera = types::make_default_model_camera();
            const auto inverse_view = glm::inverse(camera.view);
            const auto origin = glm::vec3(inverse_view[3]);
            const auto normal = glm::normalize(glm::vec3(inverse_view[2]));
            const auto ray_at = [&](const glm::vec3& point) {
                return core::CursorRay{{origin, glm::normalize(point - origin)}, normal};
            };
            core::CursorPose pose(fixture.model);
            // First establish a non-neutral gaze; returning to center must release it even
            // though the cursor is still inside the model's activation region.
            pose.apply(fixture.model,
                fixture.animation,
                ray_at(center + glm::vec3(2, 0, 0)),
                SETTLE_SECONDS);
            pose.restore(fixture.animation);
            pose.apply(fixture.model, fixture.animation, ray_at(center), SETTLE_SECONDS);
            for (const auto bone : {Neck, Head}) {
                check(
                    glm::length(fixture.animation.animation[bone].rotation - IDENTITY) < TOLERANCE,
                    "A cursor at the projected face center must restore neutral neck and head");
            }
            pose.restore(fixture.animation);
            for (const auto axis : {glm::vec3(1, 0, 0), glm::vec3(0, 1, 0)}) {
                std::array<glm::quat, 2> previous{IDENTITY, IDENTITY};
                for (const float sign : {-1.0f, 1.0f}) {
                    pose.apply(fixture.model,
                        fixture.animation,
                        ray_at(center + axis * sign * CROSSING_STEP),
                        SETTLE_SECONDS);
                    std::size_t joint = 0;
                    for (const auto bone : {Neck, Head}) {
                        const auto rotation = fixture.animation.animation[bone].rotation;
                        check(glm::length(rotation - previous[joint]) < CONTINUITY_TOLERANCE,
                            "Crossing the face center must not flip gaze between angular limits");
                        previous[joint++] = rotation;
                    }
                    pose.restore(fixture.animation);
                }
            }
            // Exercise the same pixel -> ray conversion used by the application as well.
            const auto clip = camera.mvp * glm::vec4(center, 1);
            const auto ndc = glm::vec2(clip) / clip.w;
            const types::WindowSize size{600, 800};
            const types::ClientMousePosition pixel{
                static_cast<int>(std::lround((ndc.x + 1.0f) * 0.5f * size.width)),
                static_cast<int>(std::lround((1.0f - ndc.y) * 0.5f * size.height))};
            const core::CursorRay screen_ray{
                types::Ray::make_ray_from_screen(size, pixel, camera), normal};
            pose.apply(fixture.model, fixture.animation, screen_ray, SETTLE_SECONDS);
            check(glm::length(fixture.animation.animation[Head].rotation - IDENTITY) <
                      CONTINUITY_TOLERANCE,
                "A pixel near the face center must produce nearly neutral gaze");
        }
    }
} // namespace

int main() {
    look_and_reach();
    smoothing_and_return();
    limits_and_missing_bones();
    animated_pose();
    skinning_and_physics_pose();
    body_avoidance_and_hinge();
    rotated_body_avoidance();
    release_to_authored_elbow();
    configurable_limits();
    bounded_reach_speed();
    face_center_gaze();
    overlapping_hand_tracking();
    std::cout << "Cursor pose tests passed\n";
}
