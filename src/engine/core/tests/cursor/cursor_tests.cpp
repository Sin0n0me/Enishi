#include <chrono>
#include <collider/collider.h>
#include <collider/obb_maker.h>
#include <core/system/animation/cursor_pose.h>
#include <core/system/skinning/skinning_system.h>
#include <cstdlib>
#include <engine_types/renderer/uniform_buffer/camera.h>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <sstream>
#include <string_view>

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

    void skinning_and_physics_pose(bool jumping = false) {
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
        core::CursorPoseConfig config;
        config.jump_delay = 0;
        core::CursorPose pose(model, config);
        core::SkinningSystem skinning(registry, nullptr);
        if (jumping) {
            pose.apply(model, animation, cursor(0, 11.5f), 0.05f);
            pose.restore(animation);
            pose.apply(model, animation, cursor(0, 11.5f), 0.05f);
            check(animation.animation[Root].position.y > 0, "Integration pose must be airborne");
        } else {
            pose.apply(model, animation, cursor(3, 7), SETTLE_SECONDS);
        }
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

    void hands_and_clothing_clearance(bool physics_clothing, bool attached = false) {
        Fixture fixture;
        types::BoneIndex clothing = Root;
        if (physics_clothing || attached) {
            clothing = Count;
            fixture.model.bone_node.emplace_back();
            const auto parent = attached ? Wrist : Root;
            fixture.model.bone_node.back().parent = parent;
            fixture.model.bone_node[parent].children.push_back(clothing);
            fixture.model.bone_names.push_back("garment");
            fixture.model.bind_bone.emplace_back();
            fixture.animation.animation.emplace_back();
            // The source pose is behind the body, but the completed physics pose is in front.
            fixture.animation.animation.back().position =
                attached ? glm::vec3(0) : glm::vec3(0, 7, 3);
            fixture.animation.global.push_back(
                attached ? global(fixture.model, fixture.animation, clothing)
                         : glm::translate(glm::mat4(1), glm::vec3(0, 7, -1.1f)));
            types::PhysicsRigidBody body{};
            body.relate_bone_index = clothing;
            body.kind = attached ? types::RigidBodyKind::Kinematic : types::RigidBodyKind::Dynamic;
            fixture.model.rigid_bodies.push_back(body);
        }
        fixture.model.collision_vertices.resize(fixture.model.bone_node.size());
        const auto box = [](const glm::vec3& center, const glm::vec3& extent) {
            std::vector<glm::vec3> vertices;
            for (const float x : {-1.0f, 1.0f}) {
                for (const float y : {-1.0f, 1.0f}) {
                    for (const float z : {-1.0f, 1.0f}) {
                        vertices.push_back(center + extent * glm::vec3(x, y, z));
                    }
                }
            }
            return vertices;
        };
        // A garment protrudes in front of the torso capsule and covers the old hand targets.
        fixture.model.collision_vertices[clothing] =
            box(physics_clothing ? glm::vec3(0) : glm::vec3(0, 7, -1.1f), {1.3f, 1.5f, 0.3f});
        fixture.model.collision_vertices[Wrist] = box({0.4f, 0, 0}, {0.4f, 0.2f, 0.3f});
        fixture.model.collision_vertices[OtherWrist] = box({-0.4f, 0, 0}, {0.4f, 0.2f, 0.3f});
        if (attached) {
            fixture.model.collision_vertices[clothing] = fixture.model.collision_vertices[Wrist];
        }
        std::array<types::OBB, 3> local;
        const std::array<types::BoneIndex, 3> BONES{clothing, Wrist, OtherWrist};
        for (std::size_t index = 0; index < BONES.size(); ++index) {
            auto bounds = collider::OBBMaker::make_by_covariance_matrix(
                fixture.model.collision_vertices[BONES[index]]);
            check(bounds.is_some(), "Synthetic clothing and hand OBBs must be valid");
            local[index] = bounds.unwrap();
        }
        core::CursorPoseConfig config;
        config.experimental_obb_collision = true;
        core::CursorPose pose(fixture.model, config);
        constexpr int FRAMES = 240;
        constexpr float FRAME_SECONDS = 1.0f / 60.0f;
        int both_reaching = 0;
        for (int frame = 0; frame < FRAMES; ++frame) {
            const bool release = frame > FRAMES / 2;
            const auto target = release ? std::optional<core::CursorRay>{} : cursor(0, 7);
            pose.apply(fixture.model, fixture.animation, target, FRAME_SECONDS);
            std::array<types::OBB, 3> world;
            for (std::size_t index = 0; index < BONES.size(); ++index) {
                const auto transform = physics_clothing && index == 0
                                           ? fixture.animation.global[clothing]
                                           : global(fixture.model, fixture.animation, BONES[index]);
                auto bounds = collider::OBBMaker::transform(local[index], transform);
                check(bounds.is_some(), "Synthetic world bounds must be valid");
                world[index] = bounds.unwrap();
            }
            check(collider::Collider::intersect(world[1], world[2]).is_none(),
                "Hand OBBs must not overlap during approach or release");
            for (std::size_t hand = 1; hand < world.size(); ++hand) {
                if (attached && hand == 1) {
                    check(collider::Collider::intersect(world[0], world[hand]).is_some(),
                        "Direct parent/child OBBs may overlap without blocking arm movement");
                    continue;
                }
                check(collider::Collider::intersect(world[0], world[hand]).is_none(),
                    "Hands must avoid clothing outside the torso capsule during interpolation");
            }
            if (!release &&
                glm::length(fixture.animation.animation[Arm].rotation - IDENTITY) > TOLERANCE &&
                glm::length(fixture.animation.animation[OtherArm].rotation - IDENTITY) >
                    TOLERANCE) {
                ++both_reaching;
            }
            check_arm_safety(fixture);
            pose.restore(fixture.animation);
        }
        check(both_reaching > FRAMES / 4, "Collision avoidance must preserve two-handed reaching");
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
    Fixture leg_fixture() {
        Fixture fixture;
        constexpr std::size_t LEG_BONES = 8;
        const std::array<const char*, LEG_BONES> names{"leftUpperLeg",
            "leftLowerLeg",
            "leftFoot",
            "rightUpperLeg",
            "rightLowerLeg",
            "rightFoot",
            "LeftFootIK",
            "RightFootIK"};
        const std::array<types::BoneIndex, LEG_BONES> parents{
            Root, Count, Count + 1, Root, Count + 3, Count + 4, Root, Root};
        const std::array<glm::vec3, LEG_BONES> positions{glm::vec3(1, 6, 0),
            glm::vec3(0, -3, 0),
            glm::vec3(0, -3, 0),
            glm::vec3(-1, 6, 0),
            glm::vec3(0, -3, 0),
            glm::vec3(0, -3, 0),
            glm::vec3(1, 0, 0),
            glm::vec3(-1, 0, 0)};
        fixture.model.bone_node.resize(Count + LEG_BONES);
        fixture.model.bind_bone.resize(Count + LEG_BONES);
        fixture.animation.animation.resize(Count + LEG_BONES);
        fixture.animation.global.resize(Count + LEG_BONES);
        for (std::size_t i = 0; i < LEG_BONES; ++i) {
            const auto bone = Count + i;
            fixture.model.bone_names.emplace_back(names[i]);
            fixture.model.bone_node[bone].parent = parents[i];
            fixture.model.bone_node[parents[i]].children.push_back(bone);
            fixture.animation.animation[bone].position = positions[i];
            const auto matrix = global(fixture.model, fixture.animation, bone);
            fixture.animation.global[bone] = matrix;
            fixture.model.bind_bone[bone].local = glm::translate(glm::mat4(1), positions[i]);
            fixture.model.bind_bone[bone].global = matrix;
            fixture.model.bind_bone[bone].global_inverse = glm::inverse(matrix);
        }
        for (std::size_t side = 0; side < 2; ++side) {
            const auto hip = static_cast<types::BoneIndex>(Count + side * 3);
            types::CCDIK ccd{};
            ccd.iterations = 40;
            ccd.target = hip + 2;
            ccd.ik_bone = static_cast<types::BoneIndex>(Count + 6 + side);
            ccd.chain = {hip + 1, hip};
            ccd.limit = types::IKLimitAngle{0.5f};
            ccd.link_limits = {{true, {-glm::pi<float>(), 0, 0}, {0, 0, 0}}, {}};
            fixture.model.iks.push_back({ccd});
        }
        return fixture;
    }

    void jump_knee_bend() {
        auto fixture = leg_fixture();
        auto registry = std::make_shared<ecs::Registry>();
        const auto entity = registry->create();
        check(registry->emplace<component::ModelComponent>(entity, fixture.model).is_ok(),
            "Insert leg model");
        check(registry->emplace<component::AnimationComponent>(entity, fixture.animation).is_ok(),
            "Insert leg animation");
        check(
            registry->emplace<component::SkinningComponent>(entity).is_ok(), "Insert leg skinning");
        component::IKComponent ik;
        ik.iks = fixture.model.iks;
        ik.rotation.resize(fixture.model.bone_node.size(), IDENTITY);
        ik.globals = fixture.animation.global;
        for (std::size_t i = 0; i < ik.iks.size(); ++i) {
            ik.ik_map.emplace(
                std::get<types::CCDIK>(ik.iks[i].method).ik_bone, static_cast<types::IkIndex>(i));
        }
        check(registry->emplace<component::IKComponent>(entity, ik).is_ok(), "Insert leg IK");
        auto& animation = registry->get<component::AnimationComponent>(entity).unwrap_mut();
        auto& model = registry->get<component::ModelComponent>(entity).unwrap_mut();
        core::CursorPoseConfig config;
        config.jump_delay = 0;
        core::CursorPose pose(model, config);
        core::SkinningSystem skinning(registry, nullptr);
        constexpr float DT = 1.0f / 60.0f;
        constexpr int FRAMES = 100;
        bool prepared = false;
        bool airborne = false;
        bool landed = false;
        for (int frame = 0; frame < FRAMES; ++frame) {
            pose.apply(
                model, animation, frame == 0 ? std::optional(cursor(0, 11.5f)) : std::nullopt, DT);
            const float height = animation.animation[Root].position.y;
            if (height < -TOLERANCE) {
                prepared = prepared || !airborne;
                landed = landed || airborne;
                for (const auto knee : {Count + 1, Count + 4}) {
                    check(glm::length(animation.animation[knee].rotation - IDENTITY) > TOLERANCE,
                        "Crouching must bend both knees");
                }
            }
            airborne = airborne || height > TOLERANCE;
            skinning.update(types::DeltaTime(DT));
            for (const auto foot : {Count + 2, Count + 5}) {
                const auto position = glm::vec3(animation.global[foot][3]);
                const auto bind = glm::vec3(model.bind_bone[foot].global[3]);
                check(glm::length(glm::vec2(position.x - bind.x, position.z - bind.z)) < TOLERANCE,
                    "Feet must not slide when crouching, including after foot IK");
                check(std::abs(position.y - (std::max)(height, 0.0f)) < TOLERANCE,
                    "Feet must remain planted during preparation and landing");
            }
            pose.restore(animation);
            for (std::size_t bone = 0; bone < animation.animation.size(); ++bone) {
                check(animation.animation[bone].position ==
                              fixture.animation.animation[bone].position &&
                          animation.animation[bone].rotation ==
                              fixture.animation.animation[bone].rotation,
                    "Jump must restore all source bones and IK goals");
            }
        }
        check(prepared && airborne && landed, "Jump must prepare, fly and absorb landing");
    }

    void reach_after_landing() {
        auto jumping = leg_fixture();
        auto waiting = leg_fixture();
        core::CursorPoseConfig config;
        config.jump_delay = 0;
        config.jump_cooldown = 10;
        core::CursorPose jump_pose(jumping.model, config);
        config.jump_delay = 10;
        core::CursorPose waiting_pose(waiting.model, config);
        constexpr float DT = 1.0f / 60.0f;
        constexpr int TRACK_FRAMES = 180;
        constexpr int SETTLED_FRAME = 120;
        constexpr int RELEASE_FRAMES = 120;
        constexpr float POSE_TOLERANCE = 0.02f;
        for (int frame = 0; frame < TRACK_FRAMES + RELEASE_FRAMES; ++frame) {
            const auto target =
                frame < TRACK_FRAMES ? std::optional(cursor(0, 11.5f)) : std::nullopt;
            jump_pose.apply(jumping.model, jumping.animation, target, DT);
            waiting_pose.apply(waiting.model, waiting.animation, target, DT);
            if (frame > SETTLED_FRAME && frame < TRACK_FRAMES) {
                check(jumping.animation.animation[Root].position == glm::vec3(0),
                    "Reach regression must cover the grounded cooldown");
                for (const auto arm : {Arm, OtherArm}) {
                    check(glm::length(waiting.animation.animation[arm].rotation - IDENTITY) >
                              POSE_TOLERANCE,
                        "Reference pose must keep reaching upward");
                    check(glm::length(jumping.animation.animation[arm].rotation -
                                      waiting.animation.animation[arm].rotation) < POSE_TOLERANCE,
                        "Arms must keep reaching after landing, including during cooldown");
                }
            }
            if (frame == TRACK_FRAMES + RELEASE_FRAMES - 1) {
                for (const auto arm : {Arm, OtherArm}) {
                    check(glm::length(jumping.animation.animation[arm].rotation - IDENTITY) <
                              POSE_TOLERANCE,
                        "Arms must still release when the cursor leaves");
                }
            }
            jump_pose.restore(jumping.animation);
            waiting_pose.restore(waiting.animation);
        }
    }

    void check_hanging_limbs(
        const Fixture& fixture, std::size_t selected_side, types::BoneIndex wrist) {
        const auto shoulder = selected_side == 0 ? Arm : OtherArm;
        const auto elbow = selected_side == 0 ? Elbow : OtherElbow;
        const auto upper = glm::vec3(global(fixture.model, fixture.animation, elbow)[3] -
                                     global(fixture.model, fixture.animation, shoulder)[3]);
        const auto lower = glm::vec3(global(fixture.model, fixture.animation, wrist)[3] -
                                     global(fixture.model, fixture.animation, elbow)[3]);
        check(glm::dot(glm::normalize(upper), glm::normalize(lower)) > 0.999f,
            "The supporting arm is fully extended without hyperextension");
        for (std::size_t side = 0; side < 2; ++side) {
            const auto hip = static_cast<types::BoneIndex>(Count + side * 3);
            const auto thigh = glm::vec3(global(fixture.model, fixture.animation, hip + 1)[3] -
                                         global(fixture.model, fixture.animation, hip)[3]);
            const auto shin = glm::vec3(global(fixture.model, fixture.animation, hip + 2)[3] -
                                        global(fixture.model, fixture.animation, hip + 1)[3]);
            check(glm::dot(glm::normalize(thigh), glm::normalize(shin)) > 0.999f &&
                      glm::normalize(thigh).y < -0.99f,
                "Captured crouching knees are released and both legs hang downward");
        }
    }

    void hanging_drag() {
        constexpr float DT = 1.0f / 60.0f;
        constexpr int FRAMES = 120;
        auto fixture = leg_fixture();
        constexpr float CROUCH_DEGREES = 45;
        fixture.animation.animation[Count + 1].rotation =
            glm::angleAxis(glm::radians(CROUCH_DEGREES), glm::vec3(1, 0, 0));
        fixture.animation.animation[Count + 4].rotation =
            glm::angleAxis(glm::radians(CROUCH_DEGREES), glm::vec3(1, 0, 0));
        core::CursorPoseConfig config;
        config.jump_enabled = false;
        core::CursorPose pose(fixture.model, config);
        check(!pose.pick_hand(fixture.animation, cursor(0, 10)).has_value(),
            "An idle hand cannot start a hanging drag");
        for (int frame = 0; frame < FRAMES; ++frame) {
            pose.apply(fixture.model, fixture.animation, cursor(3, 7), DT);
            for (types::BoneIndex bone = 0; bone < fixture.model.bone_node.size(); ++bone) {
                fixture.animation.global[bone] = global(fixture.model, fixture.animation, bone);
            }
            pose.restore(fixture.animation);
        }
        const auto low_hand = glm::vec3(fixture.animation.global[Wrist][3]);
        check(!pose.pick_hand(fixture.animation, cursor(low_hand.x, low_hand.y)).has_value(),
            "Dragging a hand below the head must not start suspension");
        auto entry_pose = fixture.animation.animation;
        for (int frame = 0; frame < FRAMES; ++frame) {
            pose.apply(fixture.model, fixture.animation, cursor(2, 10), DT);
            entry_pose = fixture.animation.animation;
            for (types::BoneIndex bone = 0; bone < fixture.model.bone_node.size(); ++bone) {
                fixture.animation.global[bone] = global(fixture.model, fixture.animation, bone);
            }
            pose.restore(fixture.animation);
        }
        const auto hand = glm::vec3(fixture.animation.global[Wrist][3]);
        const auto grab = cursor(hand.x, hand.y);
        const auto hit = pose.pick_hand(fixture.animation, grab);
        check(hit.has_value(), "The extended hand can be grabbed");
        pose.begin_hanging(hit->first, fixture.animation, grab);
        check(pose.is_hanging(), "Drag starts suspension");
        const auto source = fixture.animation.animation;
        const auto wrist = hit->first == 0 ? Wrist : OtherWrist;
        const glm::vec3 destination(hand.x + 2, hand.y + 3, hand.z);
        glm::quat halfway_rotation = IDENTITY;
        glm::quat completed_rotation = IDENTITY;
        constexpr int QUARTER_SECOND_FRAMES = 15;
        constexpr int HALF_SECOND_FRAMES = 30;
        constexpr float SUSPENSION_LIFT = 0.48f;
        for (int frame = 0; frame < FRAMES; ++frame) {
            pose.apply(fixture.model, fixture.animation, cursor(destination.x, destination.y), DT);
            if (frame == 0) {
                check(glm::length(fixture.animation.animation[Arm].rotation -
                                  entry_pose[Arm].rotation) < 0.01f,
                    "Hanging entry starts smoothly from the reached arm pose");
            }
            if (frame == QUARTER_SECOND_FRAMES - 1) {
                halfway_rotation = fixture.animation.animation[Root].rotation;
            }
            if (frame == HALF_SECOND_FRAMES - 1) {
                completed_rotation = fixture.animation.animation[Root].rotation;
            }
            if (frame == FRAMES - 1) {
                const auto final_rotation = fixture.animation.animation[Root].rotation;
                check(glm::length(halfway_rotation - final_rotation) > 0.01f,
                    "Hanging entry is still transitioning at a quarter second");
                check(glm::length(completed_rotation - final_rotation) < 0.005f,
                    "Hanging entry completes in approximately half a second");
                check(glm::length(glm::vec3(global(fixture.model, fixture.animation, wrist)[3]) -
                                  (destination + glm::vec3(0, SUSPENSION_LIFT, 0))) < 0.001f,
                    "Suspension adds a small model-relative lift above the cursor");
                check(global(fixture.model, fixture.animation, Root)[3].y < destination.y,
                    "The body hangs below the grabbed hand");
                check_hanging_limbs(fixture, hit->first, wrist);
                for (const auto& ik : fixture.model.iks) {
                    const auto& ccd = std::get<types::CCDIK>(ik.method);
                    check(pose.suppress_hanging_ik(ccd.ik_bone),
                        "Foot IK is suppressed while the dangling pose is active");
                    check(glm::length(glm::vec3(
                              global(fixture.model, fixture.animation, ccd.target)[3] -
                              global(fixture.model, fixture.animation, ccd.ik_bone)[3])) < 0.001f,
                        "Foot IK follows the dangling foot instead of holding it on the ground");
                }
            }
            pose.restore(fixture.animation);
            for (std::size_t bone = 0; bone < source.size(); ++bone) {
                check(fixture.animation.animation[bone].position == source[bone].position &&
                          fixture.animation.animation[bone].rotation == source[bone].rotation,
                    "Hanging never accumulates edits into the source animation");
            }
        }
        for (int frame = 0; frame < FRAMES; ++frame) {
            pose.apply(fixture.model, fixture.animation, cursor(hand.x, 7), DT);
            if (frame == FRAMES - 1) {
                check(glm::length(fixture.animation.animation[Root].position -
                                  source[Root].position) < 0.001f,
                    "Lowering the cursor below the head fades out suspension during the drag");
                check(!pose.suppress_hanging_ik(Count + 6), "Foot IK resumes below the head");
            }
            pose.restore(fixture.animation);
        }
        pose.end_hanging();
        check(!pose.is_hanging(), "Release ends the drag");
        for (int frame = 0; frame < FRAMES; ++frame) {
            pose.apply(fixture.model, fixture.animation, std::nullopt, DT);
            if (frame == FRAMES - 1) {
                check(glm::length(fixture.animation.animation[Root].position -
                                  source[Root].position) < 0.001f,
                    "Released body returns smoothly to its authored position");
            }
            pose.restore(fixture.animation);
        }
    }

    void repeated_hanging_release() {
        constexpr float DT = 1.0f / 60.0f;
        constexpr int FRAMES = 120;
        constexpr int REPETITIONS = 5;
        auto fixture = leg_fixture();
        const auto source = fixture.animation.animation;
        core::CursorPoseConfig config;
        config.jump_enabled = false;
        core::CursorPose pose(fixture.model, config);
        for (int repeat = 0; repeat < REPETITIONS; ++repeat) {
            for (int frame = 0; frame < FRAMES; ++frame) {
                pose.apply(fixture.model, fixture.animation, cursor(2, 10), DT);
                for (types::BoneIndex bone = 0; bone < fixture.model.bone_node.size(); ++bone) {
                    fixture.animation.global[bone] = global(fixture.model, fixture.animation, bone);
                }
                pose.restore(fixture.animation);
            }
            const auto hand = glm::vec3(fixture.animation.global[Wrist][3]);
            const auto grab = cursor(hand.x, hand.y);
            const auto hit = pose.pick_hand(fixture.animation, grab);
            check(hit.has_value(), "A released hand can be grabbed again without accumulated tilt");
            if (!hit.has_value()) {
                return;
            }
            pose.begin_hanging(hit->first, fixture.animation, grab);
            glm::vec3 suspended_position{};
            for (int frame = 0; frame < FRAMES; ++frame) {
                pose.apply(fixture.model, fixture.animation, cursor(hand.x + 2, hand.y + 3), DT);
                suspended_position = fixture.animation.animation[Root].position;
                pose.restore(fixture.animation);
            }
            pose.end_hanging();
            bool bent_on_landing = false;
            bool stepped_left = false;
            bool stepped_right = false;
            std::array<glm::vec3, 2> previous_feet{};
            bool have_previous_feet = false;
            constexpr int RETURN_FRAMES = 360;
            const glm::vec3 landing_offset(suspended_position.x - source[Root].position.x,
                0,
                suspended_position.z - source[Root].position.z);
            constexpr float EXPECTED_STRIDE = 2.0f;
            constexpr int LANDING_FRAMES = 36;
            constexpr int WALK_CYCLE_FRAMES = 30;
            const int expected_return_frame =
                LANDING_FRAMES +
                WALK_CYCLE_FRAMES *
                    static_cast<int>(std::ceil(glm::length(landing_offset) / EXPECTED_STRIDE));
            for (int frame = 0; frame < RETURN_FRAMES; ++frame) {
                pose.apply(fixture.model, fixture.animation, std::nullopt, DT);
                if (frame == 0) {
                    constexpr float MAX_INITIAL_DROP_RATIO = 0.05f;
                    check(glm::length(
                              fixture.animation.animation[Root].position - suspended_position) <
                              MAX_INITIAL_DROP_RATIO *
                                  glm::length(suspended_position - source[Root].position),
                        "Release descends from the displayed position without teleporting");
                }
                constexpr int CONTACT_FRAME = 18;
                constexpr int RECOVERY_FRAME = 36;
                if (frame > CONTACT_FRAME) {
                    for (const auto knee : {Count + 1, Count + 4}) {
                        const auto rotation = fixture.animation.animation[knee].rotation;
                        check(std::abs(rotation.y) < 0.0001f && std::abs(rotation.z) < 0.0001f,
                            "Landing and walking knees rotate only around local X");
                    }
                }
                if (frame > CONTACT_FRAME && frame < RECOVERY_FRAME) {
                    bent_on_landing = bent_on_landing ||
                                      glm::length(fixture.animation.animation[Count + 1].rotation -
                                                  source[Count + 1].rotation) > 0.01f;
                    check(glm::length(
                              glm::vec3(global(fixture.model, fixture.animation, Count + 2)[3]) -
                              (glm::vec3(fixture.model.bind_bone[Count + 2].global[3]) +
                                  landing_offset)) < 0.001f,
                        "Landing keeps the foot planted while bending the knee");
                }
                if (frame < RECOVERY_FRAME) {
                    const auto position = fixture.animation.animation[Root].position;
                    check(glm::length(glm::vec2(position.x - suspended_position.x,
                              position.z - suspended_position.z)) < 0.001f,
                        "Release lands in place before starting the return walk");
                } else {
                    constexpr float GROUND_TOLERANCE = 0.0001f;
                    const std::array<glm::vec3, 2> feet{
                        glm::vec3(global(fixture.model, fixture.animation, Count + 2)[3]),
                        glm::vec3(global(fixture.model, fixture.animation, Count + 5)[3])};
                    check((std::min)(std::abs(feet[0].y), std::abs(feet[1].y)) < GROUND_TOLERANCE,
                        "Side stepping keeps one foot on the ground");
                    for (std::size_t side = 0; side < feet.size(); ++side) {
                        if (have_previous_feet && std::abs(feet[side].y) < GROUND_TOLERANCE &&
                            std::abs(previous_feet[side].y) < GROUND_TOLERANCE) {
                            check(glm::length(feet[side] - previous_feet[side]) < 0.001f,
                                "The grounded foot does not slide during the return walk");
                        }
                    }
                    previous_feet = feet;
                    have_previous_feet = true;
                    stepped_left = stepped_left ||
                                   global(fixture.model, fixture.animation, Count + 2)[3].y > 0.05f;
                    stepped_right =
                        stepped_right ||
                        global(fixture.model, fixture.animation, Count + 5)[3].y > 0.05f;
                }
                if (frame == expected_return_frame) {
                    check(glm::length(fixture.animation.animation[Root].position -
                                      source[Root].position) < 0.001f,
                        "The doubled stride returns in the expected number of steps");
                }
                if (frame == RETURN_FRAMES - 1) {
                    check(glm::length(fixture.animation.animation[Root].rotation -
                                      source[Root].rotation) < 0.001f,
                        "Release restores the displayed root rotation on every repetition");
                    check(glm::length(fixture.animation.animation[Root].position -
                                      source[Root].position) < 0.001f,
                        "Side stepping returns the model to its original position");
                }
                pose.restore(fixture.animation);
                for (std::size_t bone = 0; bone < source.size(); ++bone) {
                    check(fixture.animation.animation[bone].position == source[bone].position &&
                              fixture.animation.animation[bone].rotation == source[bone].rotation,
                        "Repeated suspension preserves every source bone");
                }
            }
            check(bent_on_landing, "Releasing suspension bends the knees on landing");
            check(stepped_left && stepped_right,
                "The return walk lifts both feet in alternating steps");
        }
    }

    void hanging_landing_keeps_hips_position() {
        auto fixture = leg_fixture();
        const auto hips = static_cast<types::BoneIndex>(fixture.model.bone_node.size());
        fixture.model.bone_names.emplace_back("hips");
        fixture.model.bone_node.emplace_back();
        fixture.model.bone_node.back().parent = Root;
        fixture.model.bone_node[Root].children.push_back(hips);
        fixture.animation.animation.emplace_back();
        fixture.animation.animation.back().position = glm::vec3(0, 6, 0);
        const auto matrix = global(fixture.model, fixture.animation, hips);
        fixture.animation.global.push_back(matrix);
        fixture.model.bind_bone.emplace_back();
        fixture.model.bind_bone.back().local = matrix;
        fixture.model.bind_bone.back().global = matrix;
        fixture.model.bind_bone.back().global_inverse = glm::inverse(matrix);
        core::CursorPoseConfig config;
        config.jump_enabled = false;
        core::CursorPose pose(fixture.model, config);
        constexpr float DT = 1.0f / 60.0f;
        constexpr int FRAMES = 120;
        for (int frame = 0; frame < FRAMES; ++frame) {
            pose.apply(fixture.model, fixture.animation, cursor(2, 10), DT);
            for (types::BoneIndex bone = 0; bone < fixture.model.bone_node.size(); ++bone) {
                fixture.animation.global[bone] = global(fixture.model, fixture.animation, bone);
            }
            pose.restore(fixture.animation);
        }
        const auto hand = glm::vec3(fixture.animation.global[Wrist][3]);
        pose.begin_hanging(0, fixture.animation, cursor(hand.x, hand.y));
        glm::vec3 release_center{};
        for (int frame = 0; frame < FRAMES; ++frame) {
            pose.apply(fixture.model, fixture.animation, cursor(hand.x + 2, hand.y + 3), DT);
            release_center = glm::vec3(global(fixture.model, fixture.animation, hips)[3]);
            pose.restore(fixture.animation);
        }
        pose.end_hanging();
        constexpr int RECOVERY_FRAME = 36;
        for (int frame = 0; frame < RECOVERY_FRAME; ++frame) {
            pose.apply(fixture.model, fixture.animation, std::nullopt, DT);
            const auto center = glm::vec3(global(fixture.model, fixture.animation, hips)[3]);
            check(glm::length(glm::vec2(center.x - release_center.x, center.z - release_center.z)) <
                      0.001f,
                "Landing keeps the body in place even when the root pivot is below the hips");
            pose.restore(fixture.animation);
        }
    }

    void hanging_restart_does_not_capture_temporary_pose() {
        constexpr float DT = 1.0f / 60.0f;
        constexpr int FRAMES = 120;
        constexpr int REPETITIONS = 5;
        auto fixture = leg_fixture();
        core::CursorPoseConfig config;
        config.jump_enabled = false;
        core::CursorPose pose(fixture.model, config);
        for (int frame = 0; frame < FRAMES; ++frame) {
            pose.apply(fixture.model, fixture.animation, cursor(2, 10), DT);
            for (types::BoneIndex bone = 0; bone < fixture.model.bone_node.size(); ++bone) {
                fixture.animation.global[bone] = global(fixture.model, fixture.animation, bone);
            }
            pose.restore(fixture.animation);
        }
        const auto hand = glm::vec3(fixture.animation.global[Wrist][3]);
        const auto grab = cursor(hand.x, hand.y);
        auto expected = IDENTITY;
        // A clip can update authored bones after the last cursor evaluation.
        constexpr float AUTHORED_TWIST_DEGREES = 10;
        const auto authored_twist =
            glm::angleAxis(glm::radians(AUTHORED_TWIST_DEGREES), glm::vec3(1, 0, 0));
        fixture.animation.animation[Twist].rotation = authored_twist;
        for (int repeat = 0; repeat < REPETITIONS; ++repeat) {
            // Exercise the pose lifecycle independently of the window's drag hit-test gate.
            // Restore leaves temporary globals cached; a new drag must not capture them.
            pose.begin_hanging(0, fixture.animation, grab);
            for (int frame = 0; frame < FRAMES; ++frame) {
                pose.apply(fixture.model, fixture.animation, cursor(hand.x + 2, hand.y + 3), DT);
                if (frame == FRAMES - 1) {
                    const auto actual = fixture.animation.animation[Root].rotation;
                    check(fixture.animation.animation[Twist].rotation == authored_twist,
                        "Suspension must start from current animation, not the cached pose");
                    if (repeat == 0) {
                        expected = actual;
                    }
                    check(glm::length(actual - expected) < 0.001f,
                        "Restarting suspension must not accumulate the cached body rotation");
                }
                pose.restore(fixture.animation);
            }
            pose.end_hanging();
        }
    }

    void reachable_cursor_does_not_jump() {
        constexpr float DT = 1.0f / 60.0f;
        constexpr int FRAMES = 100;
        for (const float camera_distance : {15.0f, 25.0f, 40.0f}) {
            for (const bool frontal : {false, true}) {
                Fixture fixture;
                core::CursorPoseConfig config;
                config.jump_delay = 0;
                core::CursorPose pose(fixture.model, config);
                const auto origin = frontal ? glm::vec3(1, 10.95f, -camera_distance)
                                            : glm::vec3(0, 18.5f, -camera_distance);
                const auto normal =
                    frontal ? glm::vec3(0, 0, 1) : glm::normalize(glm::vec3(0, 11, 0) - origin);
                const glm::vec3 reachable(1, 10.95f, 0);
                const core::CursorRay ray{{origin, glm::normalize(reachable - origin)}, normal};
                for (int frame = 0; frame < FRAMES; ++frame) {
                    pose.apply(fixture.model, fixture.animation, ray, DT);
                    check(fixture.animation.animation[Root].position == glm::vec3(0),
                        "A reachable screen cursor must not trigger a jump due to target-plane "
                        "depth");
                    if (frame == FRAMES - 1) {
                        check(global(fixture.model, fixture.animation, Wrist)[3].y > 8.0f,
                            "The arm must keep reaching toward a reachable cursor");
                    }
                    pose.restore(fixture.animation);
                }
            }
        }
        Fixture fixture;
        fixture.model.bone_names.push_back("fingerTip");
        fixture.model.bone_node.emplace_back();
        fixture.model.bone_node.back().parent = Wrist;
        fixture.model.bone_node[Wrist].children.push_back(Count);
        fixture.animation.animation.emplace_back();
        fixture.animation.animation.back().position = glm::vec3(0.6f, 0, 0);
        fixture.animation.global.push_back(global(fixture.model, fixture.animation, Count));
        fixture.model.bind_bone.emplace_back();
        fixture.model.bind_bone.back().global = fixture.animation.global.back();
        core::CursorPoseConfig config;
        config.jump_delay = 0;
        core::CursorPose pose(fixture.model, config);
        for (int frame = 0; frame < FRAMES; ++frame) {
            pose.apply(fixture.model, fixture.animation, cursor(1, 11.5f), DT);
            check(fixture.animation.animation[Root].position == glm::vec3(0),
                "Finger reach beyond the wrist must also prevent a jump");
            pose.restore(fixture.animation);
        }
    }

    void stationary_jump() {
        constexpr float DT = 1.0f / 60.0f;
        constexpr int FRAMES = 120;
        for (const bool enabled : {false, true}) {
            Fixture fixture;
            // A separate foot IK root must move with the body, rather than pin feet to the floor.
            fixture.model.bone_names.push_back("FootIK");
            fixture.model.bone_node.emplace_back();
            fixture.model.bone_node.back().parent = types::INVALID_BONE_INDEX;
            fixture.model.bind_bone.emplace_back();
            fixture.model.bind_bone.back().local = glm::mat4(1);
            fixture.model.bind_bone.back().global = glm::mat4(1);
            fixture.model.bind_bone.back().global_inverse = glm::mat4(1);
            fixture.animation.animation.emplace_back();
            fixture.animation.global.emplace_back(1.0f);
            core::CursorPoseConfig config;
            config.jump_enabled = enabled;
            config.jump_delay = 0.2f;
            config.jump_duration = 0.6f;
            core::CursorPose pose(fixture.model, config);
            float peak = 0;
            int takeoffs = 0;
            float previous = 0;
            for (int frame = 0; frame < FRAMES; ++frame) {
                pose.apply(fixture.model, fixture.animation, cursor(0, 11.5f), DT);
                const auto position = fixture.animation.animation[Root].position;
                check(fixture.animation.animation.back().position == position,
                    "Independent IK root must receive the same jump translation");
                check(position.x == 0 && position.z == 0 && !(position.y < 0),
                    "Jump must stay vertical and above the source position");
                if (frame * DT < config.jump_delay) {
                    check(position.y == 0, "Jump must wait before takeoff");
                }
                if (position.y > 0 && previous == 0) {
                    ++takeoffs;
                }
                previous = position.y;
                peak = (std::max)(peak, position.y);
                pose.restore(fixture.animation);
                check(fixture.animation.animation[Root].position == glm::vec3(0),
                    "Jump must restore source translation every frame");
            }
            check(takeoffs == (enabled ? 1 : 0), "Cooldown must prevent immediate repeated jumps");
            check(enabled ? peak > 0 : peak == 0, "Jump toggle must control takeoff");
            const float skeleton_height = fixture.model.bind_bone[Head].global[3].y;
            check(!(peak > skeleton_height * config.jump_height_ratio),
                "Jump height must stay within its configured limit");
            check(previous == 0, "Jump must land at its original height");
        }
        for (const auto target : {cursor(3, 7), cursor(50, 40), cursor(0, 2)}) {
            Fixture fixture;
            core::CursorPose pose(fixture.model);
            for (int frame = 0; frame < FRAMES; ++frame) {
                pose.apply(fixture.model, fixture.animation, target, DT);
                check(fixture.animation.animation[Root].position == glm::vec3(0),
                    "Reachable, distant and low targets must not trigger a jump");
                pose.restore(fixture.animation);
            }
        }
        Fixture fixture;
        core::CursorPoseConfig config;
        config.jump_delay = 0;
        core::CursorPose pose(fixture.model, config);
        pose.apply(fixture.model, fixture.animation, cursor(0, 11.5f), DT);
        pose.restore(fixture.animation);
        float peak = 0;
        float height = 0;
        for (int frame = 0; frame < FRAMES; ++frame) {
            pose.apply(fixture.model, fixture.animation, std::nullopt, DT);
            height = fixture.animation.animation[Root].position.y;
            peak = (std::max)(peak, height);
            pose.restore(fixture.animation);
        }
        check(peak > 0 && height == 0, "Losing the cursor in flight must still finish the landing");
        std::istringstream settings("jump_enabled off\njump_height_ratio 0.2\njump_duration 0.8\n"
                                    "jump_delay 0.4\njump_cooldown 2\n");
        auto parsed = core::parse_cursor_pose_config(settings);
        check(parsed.is_ok(), "Jump configuration must parse");
        const auto loaded = parsed.unwrap();
        check(!loaded.jump_enabled && loaded.jump_height_ratio == 0.2f &&
                  loaded.jump_duration == 0.8f && loaded.jump_delay == 0.4f &&
                  loaded.jump_cooldown == 2,
            "Jump settings must be retained");
        for (const auto text : {"jump_enabled true",
                 "jump_height_ratio 0.6",
                 "jump_duration 0",
                 "jump_delay -1",
                 "jump_cooldown nan",
                 "jump_duration 11"}) {
            std::istringstream invalid(text);
            check(core::parse_cursor_pose_config(invalid).is_err(),
                "Invalid jump settings must fail");
        }
    }

    void experimental_collision_setting() {
        check(!core::CursorPoseConfig{}.experimental_obb_collision,
            "Experimental collision must default to off");
        std::istringstream empty;
        auto defaults = core::parse_cursor_pose_config(empty);
        check(defaults.is_ok() && !defaults.unwrap().experimental_obb_collision,
            "Omitted experimental collision setting must stay off");
        for (const bool enabled : {false, true}) {
            std::istringstream settings(enabled ? "experimental_obb_collision on # enabled"
                                                : "experimental_obb_collision off");
            auto parsed = core::parse_cursor_pose_config(settings);
            check(parsed.is_ok() && parsed.unwrap().experimental_obb_collision == enabled,
                "Experimental collision toggle must load");
        }
        for (const auto text : {"experimental_obb_collision",
                 "experimental_obb_collision true",
                 "experimental_obb_collision on extra",
                 "experimental_obb_collision on\nexperimental_obb_collision off"}) {
            std::istringstream settings(text);
            check(core::parse_cursor_pose_config(settings).is_err(),
                "Invalid experimental collision settings must be rejected");
        }
        Fixture plain;
        Fixture bounds;
        bounds.model.collision_vertices.resize(Count);
        constexpr float EXTENT = 10.0f;
        for (const auto bone : {Root, Wrist, OtherWrist}) {
            for (const float x : {-EXTENT, EXTENT}) {
                for (const float y : {-EXTENT, EXTENT}) {
                    for (const float z : {-EXTENT, EXTENT}) {
                        bounds.model.collision_vertices[bone].push_back({x, y, z});
                    }
                }
            }
        }
        core::CursorPose plain_pose(plain.model);
        core::CursorPose bounds_pose(bounds.model);
        constexpr int FRAMES = 60;
        // dt = 1 second / frame count.
        constexpr float FRAME_SECONDS = 1.0f / FRAMES;
        for (int frame = 0; frame < FRAMES; ++frame) {
            plain_pose.apply(plain.model, plain.animation, cursor(0, 7), FRAME_SECONDS);
            bounds_pose.apply(bounds.model, bounds.animation, cursor(0, 7), FRAME_SECONDS);
            for (std::size_t bone = 0; bone < Count; ++bone) {
                check(glm::length(plain.animation.animation[bone].rotation -
                                  bounds.animation.animation[bone].rotation) < TOLERANCE,
                    "Disabled OBBs must not affect cursor poses or hand size");
            }
            plain_pose.restore(plain.animation);
            bounds_pose.restore(bounds.animation);
        }
    }

    void overlap_query_matches_contact() {
        const types::OBB first{{0, 0, 0}, {{1, 0, 0}, {0, 1, 0}, {0, 0, 1}}, {1, 1, 1}};
        constexpr int ANGLES = 12;
        constexpr int POSITIONS = 20;
        constexpr float ANGLE_STEP = 0.17f;
        constexpr float POSITION_STEP = 0.25f;
        for (int angle = 0; angle < ANGLES; ++angle) {
            auto second = first;
            const auto rotation = glm::mat3_cast(glm::angleAxis(
                static_cast<float>(angle) * ANGLE_STEP, glm::normalize(glm::vec3(1, 2, 3))));
            for (int axis = 0; axis < 3; ++axis) {
                second.axis[axis] = rotation[axis];
            }
            for (int position = 0; position < POSITIONS; ++position) {
                second.center.x = static_cast<float>(position) * POSITION_STEP;
                check(collider::Collider::overlap(first, second) ==
                          collider::Collider::intersect(first, second).is_some(),
                    "Boolean SAT must match contact queries for rotation, containment and "
                    "touching");
            }
        }
        auto invalid = first;
        invalid.half_extent.x = -1;
        check(!collider::Collider::overlap(first, invalid), "Invalid bounds must be rejected");
        check(collider::Collider::intersect(1, first, 1, first).is_none(),
            "Geometry queries must not enable same-model collision events");
    }

    void cached_pose_changes() {
        Fixture fixture;
        core::CursorPose warm(fixture.model);
        constexpr int FRAMES = 12;
        constexpr float MOVE_STEP = 0.03f;
        for (int frame = 0; frame < FRAMES; ++frame) {
            const float phase = static_cast<float>(frame) * MOVE_STEP;
            fixture.animation.animation[Root].position = {phase, phase, 0};
            fixture.animation.animation[Root].rotation = glm::angleAxis(phase, glm::vec3(0, 1, 0));
            fixture.animation.animation[Root].scale = {1 + phase, 1, 1};
            auto fresh_animation = fixture.animation;
            core::CursorPose fresh(fixture.model);
            const auto target = cursor(0, 7);
            warm.apply(fixture.model, fixture.animation, target, SETTLE_SECONDS);
            fresh.apply(fixture.model, fresh_animation, target, SETTLE_SECONDS);
            for (const auto bone : {Neck, Head, Arm, Elbow, OtherArm, OtherElbow}) {
                check(glm::length(fixture.animation.animation[bone].rotation -
                                  fresh_animation.animation[bone].rotation) < TOLERANCE,
                    "Cached transforms must follow parent translation, rotation and scale");
            }
            warm.restore(fixture.animation);
        }
    }

    void benchmark_cursor() {
        Fixture fixture;
        constexpr int EXTRA_BONES = 192;
        constexpr int GRID_WIDTH = 16;
        constexpr int WARMUP_FRAMES = 30;
        constexpr int MEASURED_FRAMES = 300;
        constexpr float FRAME_SECONDS = 1.0f / 60.0f;
        constexpr float GRID_SPACING = 0.2f;
        constexpr float BOX_EXTENT = 0.08f;
        fixture.model.collision_vertices.resize(Count + EXTRA_BONES);
        for (int offset = 0; offset < EXTRA_BONES; ++offset) {
            const auto bone = static_cast<types::BoneIndex>(fixture.model.bone_node.size());
            fixture.model.bone_node.emplace_back();
            fixture.model.bone_node.back().parent = Root;
            fixture.model.bone_node[Root].children.push_back(bone);
            fixture.model.bone_names.push_back("synthetic");
            fixture.animation.animation.emplace_back();
            fixture.animation.animation.back().position = {
                GRID_SPACING * static_cast<float>(offset % GRID_WIDTH),
                GRID_SPACING * static_cast<float>(offset / GRID_WIDTH),
                3.0f};
            fixture.animation.global.push_back(global(fixture.model, fixture.animation, bone));
            fixture.model.bind_bone.emplace_back();
            auto& bind = fixture.model.bind_bone.back();
            bind.local = fixture.animation.global.back();
            bind.global = bind.local;
            bind.global_inverse = glm::inverse(bind.global);
            for (const float x : {-1.0f, 1.0f}) {
                for (const float y : {-1.0f, 1.0f}) {
                    for (const float z : {-1.0f, 1.0f}) {
                        fixture.model.collision_vertices[bone].push_back(
                            BOX_EXTENT * glm::vec3(x, y, z));
                    }
                }
            }
        }
        for (const auto hand : {Wrist, OtherWrist}) {
            fixture.model.collision_vertices[hand] = fixture.model.collision_vertices[Count];
        }
        core::CursorPoseConfig config;
        config.experimental_obb_collision = true;
        core::CursorPose pose(fixture.model, config);
        std::chrono::steady_clock::time_point start;
        float movement = 0;
        for (int frame = -WARMUP_FRAMES; frame < MEASURED_FRAMES; ++frame) {
            if (frame == 0) {
                start = std::chrono::steady_clock::now();
            }
            const float phase = static_cast<float>(frame) * FRAME_SECONDS;
            pose.apply(fixture.model,
                fixture.animation,
                cursor(std::sin(phase), 7.0f + std::sin(phase)),
                FRAME_SECONDS);
            movement += glm::length(fixture.animation.animation[Arm].rotation - IDENTITY);
            pose.restore(fixture.animation);
        }
        const auto elapsed =
            std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start)
                .count();
        check(movement > TOLERANCE, "The benchmark must actually animate the arms");
        std::cout << "Cursor benchmark: " << MEASURED_FRAMES << " frames, " << elapsed << " ms, "
                  << elapsed / MEASURED_FRAMES << " ms/frame\n";
    }
} // namespace

int main(int argc, char** argv) {
    if (argc == 2 && std::string_view(argv[1]) == "--benchmark") {
        benchmark_cursor();
        return EXIT_SUCCESS;
    }
    look_and_reach();
    smoothing_and_return();
    limits_and_missing_bones();
    animated_pose();
    skinning_and_physics_pose();
    skinning_and_physics_pose(true);
    body_avoidance_and_hinge();
    rotated_body_avoidance();
    release_to_authored_elbow();
    configurable_limits();
    stationary_jump();
    reachable_cursor_does_not_jump();
    hanging_drag();
    repeated_hanging_release();
    hanging_landing_keeps_hips_position();
    hanging_restart_does_not_capture_temporary_pose();
    jump_knee_bend();
    reach_after_landing();
    experimental_collision_setting();
    bounded_reach_speed();
    face_center_gaze();
    overlapping_hand_tracking();
    hands_and_clothing_clearance(false);
    hands_and_clothing_clearance(true);
    hands_and_clothing_clearance(false, true);
    overlap_query_matches_contact();
    cached_pose_changes();
    std::cout << "Cursor pose tests passed\n";
}
