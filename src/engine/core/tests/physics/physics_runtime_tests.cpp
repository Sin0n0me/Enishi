#include "../test_runner.h"
#include <cstdlib>
#include <iostream>
#include <physics/bullet3/converter/bullet_converter.h>
#include <physics/bullet3/motion_state/dynamic/mmd_dynamic_and_bone_merge_motion_state.h>
#include <physics/bullet3/motion_state/kinematic/mmd_kinematic_motion_state.h>
#include <physics/bullet3/physics_native_resource_maker.h>
#include <physics/bullet3/physics_world.h>
#include <physics/bullet3/rigid_body/rigid_body.h>
#include <physics/bullet3/rigid_body/rigid_body_impulse.h>
#include <physics/common/physics_config.h>
#include <physics/helper/helper.h>
#include <skinning_system/cache/physics_bone_cache.h>
#include <skinning_system/updater/physics_bones_updater.h>
#include <skinning_system/views/physics_bone_view.h>

using namespace enishi;
namespace {
    constexpr float TOLERANCE = 0.0001f;
    using tests::check;

    void sleeping_tests() {
        constexpr int SETTLE_FRAMES = 360;
        constexpr float STEP = 1.0f / 60.0f;
        for (const auto kind : {types::RigidBodyKind::Dynamic,
                 types::RigidBodyKind::DynamicAdjustBone,
                 types::RigidBodyKind::Kinematic}) {
            glm::mat4 local(1);
            const auto authored = glm::translate(glm::mat4(1), glm::vec3(0, 10, 0));
            auto global = authored;
            auto view = std::make_shared<skinning_system::PhysicsBoneView>(local, global);
            const std::vector<types::BoneNode> nodes(1);
            auto cache = std::make_shared<skinning_system::PhysicsBonesCache>(
                nodes, std::vector<std::shared_ptr<skinning_system::PhysicsBoneView>>{view});
            auto updater = std::make_shared<skinning_system::PhysicsBonesUpdater>(*cache);
            types::PhysicsRigidBody description{};
            description.kind = kind;
            description.mass = 1;
            description.shape = types::RBShapeSphere{0.1f};
            auto shape_result =
                physics::bullet3::PhysicsNativeResourceMaker::make_shape(description);
            check(shape_result.is_ok(), "create sleep test shape");
            auto shape = std::move(shape_result).unwrap_mut();
            auto [active, kinematic] =
                physics::bullet3::PhysicsNativeResourceMaker::make_motion_state(description, true);
            auto result = physics::bullet3::PhysicsNativeResourceMaker::make_rigid_body(
                description, shape.get(), active.get(), kinematic.get());
            check(result.is_ok(), "create sleep test body");
            auto body = std::move(result).unwrap_mut();
            physics::bullet3::BulletRigidBody wrapper(*body,
                *active,
                *kinematic,
                physics::PhysicsBoneViews{cache, updater, view},
                kind,
                0);
            wrapper.set_active(true);
            wrapper.reset_transform();
            btDefaultCollisionConfiguration collision;
            btCollisionDispatcher dispatcher(&collision);
            btDbvtBroadphase broadphase;
            btSequentialImpulseConstraintSolver solver;
            btDiscreteDynamicsWorld world(&dispatcher, &broadphase, &solver, &collision);
            world.setGravity(btVector3(0, 0, 0));
            world.addRigidBody(body.get());
            for (int frame = 0; frame < SETTLE_FRAMES; ++frame) {
                global = authored;
                wrapper.sync_animation();
                world.stepSimulation(STEP, 1, STEP);
                wrapper.apply_global_transform();
            }
            check(!body->isActive(),
                "stationary bodies must sleep despite animation synchronization");
            global = authored;
            global[3].x = 1;
            wrapper.sync_animation();
            check(body->isActive(), "changed authored pose wakes sleeping body");
            world.stepSimulation(STEP, 1, STEP);
            if (kind == types::RigidBodyKind::Kinematic) {
                check(std::abs(body->getWorldTransform().getOrigin().x() - 1) < TOLERANCE,
                    "sleeping kinematic body resumes following animation");
            } else {
                body->forceActivationState(ISLAND_SLEEPING);
                types::RigidBodyImpulse impulse;
                impulse.world_velocity = {1, 0, 0};
                physics::bullet3::apply_rigid_body_impulse(*body, impulse);
                check(body->isActive() && body->getLinearVelocity().x() > 0,
                    "click impulse wakes sleeping dynamic body");
            }
            body->forceActivationState(ISLAND_SLEEPING);
            wrapper.reset_transform();
            check(body->isActive(), "pose reset wakes sleeping body");
            world.removeRigidBody(body.get());
        }
    }

    void motion_state_tests() {
        glm::mat4 local(1);
        auto global = glm::translate(glm::mat4(1), glm::vec3(2, 3, 4));
        auto view = std::make_shared<skinning_system::PhysicsBoneView>(local, global);
        const std::vector<types::BoneNode> nodes(1);
        skinning_system::PhysicsBonesCache cache(nodes, {view});
        skinning_system::PhysicsBonesUpdater updater(cache);
        const auto offset = glm::translate(glm::mat4(1), glm::vec3(0, 1, 0));
        physics::bullet3::MMDDynamicAndBoneMergeMotionState state(offset, true, 0);
        state.reset(view.get());
        btTransform transform;
        state.getWorldTransform(transform);
        check(
            glm::length(glm::vec3(physics::inverse_z(
                            physics::bullet3::BulletConverter::transform_to_matrix(transform))[3]) -
                        glm::vec3(2, 4, 4)) < TOLERANCE,
            "merged body initializes from bone and offset");
        const auto rotation = glm::rotate(glm::mat4(1), glm::half_pi<float>(), glm::vec3(0, 0, 1));
        state.setWorldTransform(physics::bullet3::BulletConverter::matrix_to_transform(
            physics::inverse_z(rotation * offset)));
        state.reflect_global_transform(view.get(), &updater);
        check(glm::vec3(global[3]) == glm::vec3(2, 3, 4) &&
                  glm::length(glm::vec3(global[0]) - glm::vec3(0, 1, 0)) < TOLERANCE,
            "merged body keeps animated translation and simulated rotation");
        physics::bullet3::MMDKinematicMotionState kinematic(offset);
        kinematic.reset(view.get());
        global[3] = glm::vec4(5, 6, 7, 1);
        kinematic.update_global_transform(view.get());
        kinematic.getWorldTransform(transform);
        const auto actual =
            physics::inverse_z(physics::bullet3::BulletConverter::transform_to_matrix(transform));
        check(glm::length(glm::vec3(actual[3]) - glm::vec3((global * offset)[3])) < TOLERANCE,
            "kinematic state follows current animation");
    }

    void world_tests() {
        auto config = std::make_shared<physics::PhysicsWorldConfig>();
        config->set_updatable(true);
        config->set_fixed_step_time(1.0f / 60.0f);
        config->set_max_step_count(4);
        physics::bullet3::PhysicsWorld world(config);
        check(world.init().is_ok(), "initialize Bullet world");
        glm::mat4 local(1);
        auto global = glm::translate(glm::mat4(1), glm::vec3(0, 10, 0));
        auto view = std::make_shared<skinning_system::PhysicsBoneView>(local, global);
        const std::vector<types::BoneNode> nodes(1);
        auto cache = std::make_shared<skinning_system::PhysicsBonesCache>(
            nodes, std::vector<std::shared_ptr<skinning_system::PhysicsBoneView>>{view});
        auto updater = std::make_shared<skinning_system::PhysicsBonesUpdater>(*cache);
        const auto object = world.add_object();
        check(object.is_ok(), "create physics object");
        types::PhysicsRigidBody body{};
        body.kind = types::RigidBodyKind::Dynamic;
        body.shape = types::RBShapeSphere{0.1f};
        body.mass = 1;
        body.group_mask = UINT16_MAX;
        const auto handle = world.add_rigid_body(object.unwrap(), body, cache, updater, view);
        check(handle.is_ok(), "link native body and motion state to registered handles");
        check(world.apply_impulse(handle.unwrap(), {}).is_ok(), "resolve body impulse handle");
        check(world.apply_impulse(object.unwrap(), {}).is_err(), "reject object as impulse target");
        check(world.apply_impulse({}, {}).is_err(), "reject missing impulse target");
        for (int frame = 0; frame < 4; ++frame) {
            world.simulation(types::DeltaTime(1.0f / 60.0f));
            world.apply_physics();
        }
        check(
            global[3].y < 10.0f && global[3].y > 9.0f, "dynamic body updates bone in Bullet world");
        body.kind = types::RigidBodyKind::Kinematic;
        check(world.add_rigid_body(object.unwrap(), body, cache, updater, view).is_ok(),
            "kinematic body has valid motion states");
        world.simulation(types::DeltaTime(1.0f / 60.0f));
        world.apply_physics();
    }
} // namespace

int main() {
    extern void physics_object_tests();
    tests::run_test("physics_object_tests", physics_object_tests);
    extern void impulse_playback_tests();
    tests::run_test("impulse_playback_tests", impulse_playback_tests);
    extern void rigid_body_impulse_tests();
    tests::run_test("rigid_body_impulse_tests", rigid_body_impulse_tests);
    extern void model_physics_tests();
    tests::run_test("model_physics_tests", model_physics_tests);
    tests::run_test("sleeping_tests", sleeping_tests);
    tests::run_test("motion_state_tests", motion_state_tests);
    tests::run_test("world_tests", world_tests);
    std::cout << "Physics runtime tests passed\n";
}
