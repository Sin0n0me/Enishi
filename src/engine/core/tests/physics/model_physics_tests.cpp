#include <core/system/physics/physics_system.h>
#include <core/system/skinning/model_pose_initializer.h>
#include <core/system/skinning/skinning_system.h>
#include <cstdlib>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <model_controller/model_component_builder.h>
#include <physics/bullet3/physics_engine.h>
#include <platform_impl/physics/physics_config.h>

using namespace enishi;
namespace {
    void check(bool condition, const char* message) {
        if (!condition) {
            std::cerr << message << '\n';
            std::exit(EXIT_FAILURE);
        }
    }

    component::ModelComponent make_model() {
        types::ModelData data;
        types::AddonBones bones(4);
        bones[1].bone_node.parent = 0;
        bones[0].bone_node.children = {1};
        const std::array<glm::vec3, 4> positions{
            glm::vec3(0, 10, 0), glm::vec3(1, 10, 0), glm::vec3(3, 10, 0), glm::vec3(7, 10, 0)};
        for (std::size_t index = 0; index < bones.size(); ++index) {
            auto& bind = bones[index].bind_bone;
            bind.global = glm::translate(glm::mat4(1), positions[index]);
            bind.global_inverse = glm::inverse(bind.global);
            bind.local = bind.global;
        }
        bones[1].bind_bone.local = glm::translate(glm::mat4(1), glm::vec3(1, 0, 0));
        data.addons.emplace_back(bones);
        types::AddonBoneConstraints constraints;
        types::BoneConstraint inherited;
        inherited.bone = 2;
        inherited.source = 0;
        inherited.translation_weight = 1;
        inherited.after_physics = true;
        constraints.constraints.push_back(inherited);
        data.addons.emplace_back(constraints);
        types::PhysicsRigidBody body{};
        body.kind = types::RigidBodyKind::Dynamic;
        body.mass = 1;
        body.shape = types::RBShapeSphere{0.1f};
        body.group_mask = UINT16_MAX;
        types::AddonRigidBodies bodies{body};
        body.relate_bone_index = 3;
        body.kind = types::RigidBodyKind::Kinematic;
        bodies.push_back(body);
        data.addons.emplace_back(bodies);
        return model_controller::make_model_component(data, {});
    }
} // namespace

void model_physics_tests() {
    constexpr float TOLERANCE = 0.0001f;
    auto registry = std::make_shared<ecs::Registry>();
    const auto entity = registry->create();
    auto model = make_model();
    check(registry->insert(entity, model).is_ok(), "insert simulated model");
    check(core::initialize_model_pose(*registry, entity, model).is_ok(), "register model physics");
    check(registry->has<component::PhysicsBodiesComponent>(entity) &&
              registry->has<component::PhysicsComponent>(entity),
        "physics addons become runtime components");
    auto config = std::make_shared<platform_impl::PhysicsWorldConfig>();
    config->set_updatable(true);
    auto engine = std::make_shared<physics::bullet3::PhysicsEngine>(config);
    check(engine->init_world().is_ok(), "initialize model physics world");
    auto skinning = std::make_shared<core::SkinningSystem>(registry, engine);
    core::PhysicsSystem physics(registry, engine, skinning);
    auto& animation = registry->get<component::AnimationComponent>(entity).unwrap_mut();
    animation.animation[1].position.y = 1;
    animation.animation[3].position.y = 11;
    const types::DeltaTime dt(1.0f / 60.0f);
    for (int frame = 0; frame < 4; ++frame) {
        skinning->update(dt);
        physics.update(dt);
    }
    const auto root = glm::vec3(animation.global[0][3]);
    check(root.y < 10 && root.y > 9, "same-frame physics result reaches animation");
    check(glm::length(glm::vec3(animation.global[1][3]) - root - glm::vec3(1, 1, 0)) < TOLERANCE,
        "simulated parent preserves child animation");
    check(glm::length(glm::vec3(animation.global[2][3]) - glm::vec3(3, root.y, 0)) < TOLERANCE,
        "after-physics inheritance observes current simulated source");
    check(glm::vec3(animation.global[3][3]) == glm::vec3(7, 11, 0),
        "kinematic bodies do not replace animation");
    const auto& matrices =
        registry->get<component::SkinningComponent>(entity).unwrap().skinning_matrices;
    check(glm::length(glm::vec3(matrices[0] * glm::vec4(0, 10, 0, 1)) - root) < TOLERANCE,
        "physics reaches current frame skinning matrix");
    config->set_updatable(false);
    animation.animation[0].position.y = 20;
    skinning->update(dt);
    physics.update(dt);
    check(animation.global[0][3].y == 20 && animation.global[2][3].y == 20,
        "disabled simulation uses animation and still evaluates after-physics constraints");
}
