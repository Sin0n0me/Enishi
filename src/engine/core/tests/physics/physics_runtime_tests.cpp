#include <cstdlib>
#include <iostream>
#include <physics/bullet3/converter/bullet_converter.h>
#include <physics/bullet3/motion_state/dynamic/mmd_dynamic_and_bone_merge_motion_state.h>
#include <physics/bullet3/motion_state/kinematic/mmd_kinematic_motion_state.h>
#include <physics/bullet3/physics_world.h>
#include <physics/helper/helper.h>
#include <platform_impl/physics/physics_config.h>
#include <skinning_system/cache/physics_bone_cache.h>
#include <skinning_system/updater/physics_bones_updater.h>
#include <skinning_system/views/physics_bone_view.h>

using namespace enishi;
namespace {
    constexpr float TOLERANCE = 0.0001f;
    void check(bool condition, const char* message) {
        if (!condition) {
            std::cerr << message << '\n';
            std::exit(EXIT_FAILURE);
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
        auto config = std::make_shared<platform_impl::PhysicsWorldConfig>();
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
    extern void rigid_body_impulse_tests();
    rigid_body_impulse_tests();
    extern void model_physics_tests();
    model_physics_tests();
    motion_state_tests();
    world_tests();
    std::cout << "Physics runtime tests passed\n";
}
