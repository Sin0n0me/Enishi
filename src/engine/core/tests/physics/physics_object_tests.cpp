#include "../test_runner.h"
#include <cstdlib>
#include <iostream>
#include <physics/bullet3/physics_world.h>
#include <physics/common/physics_config.h>
#include <skinning_system/cache/physics_bone_cache.h>
#include <skinning_system/updater/physics_bones_updater.h>
#include <skinning_system/views/physics_bone_view.h>

using namespace enishi;
namespace {
    using tests::check;
    class CountingUpdater : public skinning_system::PhysicsBonesUpdater {
      public:
        int updates = 0;
        explicit CountingUpdater(skinning_system::PhysicsBonesCache& cache)
            : PhysicsBonesUpdater(cache) {
        }
        void update_local(types::BoneIndex index) noexcept override {
            ++this->updates;
            PhysicsBonesUpdater::update_local(index);
        }
    };

    void sleeping_joint_tests() {
        constexpr float STEP = 1.0f / 60.0f;
        constexpr int SETTLE_FRAMES = 360;
        glm::mat4 anchor_local(1);
        glm::mat4 dynamic_local(1);
        auto anchor_global = glm::translate(glm::mat4(1), glm::vec3(0, 10, 0));
        auto dynamic_global = anchor_global;
        auto anchor =
            std::make_shared<skinning_system::PhysicsBoneView>(anchor_local, anchor_global);
        auto dynamic =
            std::make_shared<skinning_system::PhysicsBoneView>(dynamic_local, dynamic_global);
        const std::vector<types::BoneNode> nodes(2);
        auto cache = std::make_shared<skinning_system::PhysicsBonesCache>(
            nodes, std::vector<std::shared_ptr<skinning_system::PhysicsBoneView>>{anchor, dynamic});
        auto updater = std::make_shared<skinning_system::PhysicsBonesUpdater>(*cache);
        physics::bullet3::PhysicsWorld world(std::make_shared<physics::PhysicsWorldConfig>());
        check(world.init().is_ok(), "initialize joint sleep world");
        world.set_gravity(glm::vec3(0));
        const auto object = world.add_object();
        check(object.is_ok(), "create joint sleep object");
        types::PhysicsRigidBody body{};
        body.kind = types::RigidBodyKind::Kinematic;
        body.shape = types::RBShapeSphere{0.1f};
        check(world.add_rigid_body(object.unwrap(), body, cache, updater, anchor).is_ok(),
            "create joint anchor");
        body.kind = types::RigidBodyKind::Dynamic;
        body.mass = 1;
        body.relate_bone_index = 1;
        check(world.add_rigid_body(object.unwrap(), body, cache, updater, dynamic).is_ok(),
            "create disconnected bone attached by joint only");
        types::PhysicsJoint joint{};
        joint.rigid_body_b = 1;
        joint.position = {0, 10, 0};
        check(world.add_joint(object.unwrap(), joint).is_ok(), "create locked joint");
        for (int frame = 0; frame < SETTLE_FRAMES; ++frame) {
            world.simulation(types::DeltaTime(STEP));
        }
        anchor_global[3].x = 1;
        for (int frame = 0; frame < 30; ++frame) {
            world.simulation(types::DeltaTime(STEP));
        }
        world.apply_physics();
        check(dynamic_global[3].x > 0.5f,
            "moving sleeping anchor wakes joint-only attachment without bone animation changes");
    }

    void enabled_body_cache_tests() {
        constexpr int BODY_COUNT = 64;
        glm::mat4 local(1);
        glm::mat4 global(1);
        const std::vector<types::BoneNode> nodes(1);
        auto view = std::make_shared<skinning_system::PhysicsBoneView>(local, global);
        auto cache = std::make_shared<skinning_system::PhysicsBonesCache>(
            nodes, std::vector<std::shared_ptr<skinning_system::PhysicsBoneView>>{view});
        auto updater = std::make_shared<CountingUpdater>(*cache);
        auto config = std::make_shared<physics::PhysicsWorldConfig>();
        physics::bullet3::PhysicsWorld world(config);
        check(world.init().is_ok(), "initialize cached body world");
        const auto object = world.add_object();
        check(object.is_ok(), "create cached body object");
        types::PhysicsRigidBody body{};
        body.kind = types::RigidBodyKind::Dynamic;
        body.mass = 1;
        body.shape = types::RBShapeSphere{0.1f};
        std::vector<types::PhysicsHandle> handles;
        for (int index = 0; index < BODY_COUNT; ++index) {
            global[3].y = static_cast<float>(index + 1);
            const auto result = world.add_rigid_body(object.unwrap(), body, cache, updater, view);
            check(result.is_ok(), "grow pools without invalidating cached references");
            handles.push_back(result.unwrap());
            if (index == 0) {
                world.apply_physics();
                check(updater->updates == 1, "populate enabled cache before growing the pools");
                updater->updates = 0;
            }
        }
        world.apply_physics();
        check(updater->updates == BODY_COUNT && global[3].y == BODY_COUNT,
            "apply all bodies in registration order");
        for (const auto handle : handles) {
            check(world.set_body_enabled(handle, false).is_ok(), "disable cached body");
        }
        updater->updates = 0;
        global[3].y = 100;
        world.simulation(types::DeltaTime(1.0f / 60.0f));
        world.apply_physics();
        check(updater->updates == 0 && global[3].y == 100,
            "hidden bodies neither update bones nor overwrite the authored pose");
        check(world.set_body_enabled(handles.back(), true).is_ok(), "enable last body first");
        global[3].y = 200;
        check(world.set_body_enabled(handles.front(), true).is_ok(), "enable first body last");
        check(
            world.set_body_enabled(handles.front(), true).is_ok(), "repeated enable is idempotent");
        world.apply_physics();
        check(updater->updates == 2 && global[3].y == 100,
            "reenabling preserves original order and introduces no duplicate updates");
        updater->updates = 0;
        check(world.set_body_enabled(handles.back(), false).is_ok(), "disable after cache refresh");
        world.apply_physics();
        check(updater->updates == 1 && global[3].y == 200,
            "apply without simulation also refreshes the enabled list");
    }
} // namespace

void physics_object_tests() {
    sleeping_joint_tests();
    enabled_body_cache_tests();
    glm::mat4 local(1);
    glm::mat4 global(1);
    const std::vector<types::BoneNode> nodes(1);
    std::weak_ptr<skinning_system::PhysicsBoneView> released_view;
    {
        auto config = std::make_shared<physics::PhysicsWorldConfig>();
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
