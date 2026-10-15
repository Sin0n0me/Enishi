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

    void enabled_body_cache_tests() {
        constexpr int BODY_COUNT = 64;
        glm::mat4 local(1);
        glm::mat4 global(1);
        const std::vector<types::BoneNode> nodes(1);
        auto view = std::make_shared<skinning_system::PhysicsBoneView>(local, global);
        auto cache = std::make_shared<skinning_system::PhysicsBonesCache>(
            nodes, std::vector<std::shared_ptr<skinning_system::PhysicsBoneView>>{view});
        auto updater = std::make_shared<CountingUpdater>(*cache);
        auto config = std::make_shared<platform_impl::PhysicsWorldConfig>();
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
    enabled_body_cache_tests();
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
