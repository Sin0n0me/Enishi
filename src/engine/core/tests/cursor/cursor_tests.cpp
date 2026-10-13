#include <core/system/animation/cursor_pose.h>
#include <core/system/skinning/skinning_system.h>
#include <cstdlib>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

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
                  glm::radians(20.0f) + TOLERANCE,
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
        check(glm::length(wrist - shoulder) < 4.0f, "Unreachable targets must not stretch the arm");
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
} // namespace

int main() {
    look_and_reach();
    smoothing_and_return();
    limits_and_missing_bones();
    animated_pose();
    skinning_and_physics_pose();
    std::cout << "Cursor pose tests passed\n";
}
