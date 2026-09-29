#include <cstdlib>
#include <iostream>
#include <physics/bullet3/physics_world.h>
#include <platform_impl/physics/physics_config.h>
#include <skinning_system/cache/physics_bone_cache.h>
#include <skinning_system/updater/physics_bones_updater.h>
#include <skinning_system/views/physics_bone_view.h>

using namespace enishi;
namespace {
    void check(bool condition, const char* message) {
        if (!condition) {
            std::cerr << message << '\n';
            std::exit(EXIT_FAILURE);
        }
    }
} // namespace

void physics_object_tests() {
    glm::mat4 local(1);
    glm::mat4 global(1);
    const std::vector<types::BoneNode> nodes(1);
    std::weak_ptr<skinning_system::PhysicsBoneView> released_view;
    {
        auto config = std::make_shared<platform_impl::PhysicsWorldConfig>();
        physics::bullet3::PhysicsWorld world(config);
        check(world.init().is_ok(), "initialize multiple-object world");
        auto view = std::make_shared<skinning_system::PhysicsBoneView>(local, global);
        released_view = view;
        auto cache = std::make_shared<skinning_system::PhysicsBonesCache>(
            nodes, std::vector<std::shared_ptr<skinning_system::PhysicsBoneView>>{view});
        auto updater = std::make_shared<skinning_system::PhysicsBonesUpdater>(*cache);
        types::PhysicsRigidBody body{};
        body.kind = types::RigidBodyKind::Dynamic;
        body.mass = 1;
        body.shape = types::RBShapeSphere{0.1f};
        check(world.add_rigid_body({}, body, cache, updater, view).is_err(),
            "reject missing object before registering a body");
        for (int model = 0; model < 2; ++model) {
            const auto object = world.add_object();
            check(object.is_ok(), "register separate physics objects");
            for (int index = 0; index < 2; ++index) {
                check(world.add_rigid_body(object.unwrap(), body, cache, updater, view).is_ok(),
                    "register bodies after earlier model handles");
            }
            types::PhysicsJoint joint{};
            joint.rigid_body_b = 1;
            check(world.add_joint(object.unwrap(), joint).is_ok(),
                "joint indices resolve within each model's own body list");
        }
    }
    check(
        released_view.expired(), "world destruction releases body views without ownership cycles");
}
