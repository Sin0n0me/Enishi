#include <assets_system/model/model_loader/pmx/pmx_to_model_data.h>
#include <component/morph_component.h>
#include <core/system/physics/physics_system.h>
#include <core/system/skinning/model_pose_initializer.h>
#include <core/system/skinning/skinning_system.h>
#include <cstdlib>
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

    component::ModelComponent impulse_model() {
        constexpr std::uint8_t IMPULSE_MORPH = 10;
        constexpr std::uint8_t DYNAMIC_BODY = 1;
        assets_system::PMXData data;
        data.version = 2.1f;
        data.bones.emplace_back().position = {0, 10, 0};
        data.vertices.emplace_back().bones[0] = 0;
        data.vertices.front().weights[0] = 1;
        auto& body = data.rigid_bodies.emplace_back();
        body.bone = 0;
        body.mode = DYNAMIC_BODY;
        body.mass = 1;
        body.size = {0.1f, 0, 0};
        body.position = {0, 10, 0};
        data.morphs.resize(2);
        for (auto& morph : data.morphs) {
            morph.type = IMPULSE_MORPH;
            morph.offsets.emplace_back().index = 0;
        }
        data.morphs[0].offsets.front().translation = {0, 6, 0};
        const auto converted =
            assets_system::PMXToModelData::to_model_data("impulse.pmx", data, nullptr);
        check(converted.is_ok(), "convert PMX impulse and reset morphs");
        return model_controller::make_model_component(*converted.unwrap(), {});
    }
} // namespace

void impulse_playback_tests() {
    constexpr float TOLERANCE = 0.0001f;
    auto model = impulse_model();
    auto registry = std::make_shared<ecs::Registry>();
    const auto entity = registry->create();
    check(registry->insert(entity, model).is_ok(), "insert impulse model");
    check(
        core::initialize_model_pose(*registry, entity, model).is_ok(), "initialize impulse model");
    auto config = std::make_shared<platform_impl::PhysicsWorldConfig>();
    config->set_updatable(true);
    auto engine = std::make_shared<physics::bullet3::PhysicsEngine>(config);
    check(engine->init_world().is_ok(), "initialize impulse world");
    engine->get_world()->set_gravity(glm::vec3(0));
    auto skinning = std::make_shared<core::SkinningSystem>(registry, engine);
    core::PhysicsSystem physics(registry, engine, skinning);
    auto& weights = registry->get<component::MorphComponent>(entity).unwrap_mut().weights;
    const auto& pose = registry->get<component::AnimationComponent>(entity).unwrap();
    const types::DeltaTime dt(1.0f / 60.0f);
    const auto frame = [&] {
        skinning->update(dt);
        physics.update(dt);
    };
    weights[0] = 1;
    frame();
    weights[0] = 0;
    for (int index = 0; index < 3; ++index) {
        frame();
    }
    check(pose.global[0][3].y > 10.1f, "morph impulse reaches simulated body and skinning pose");
    weights[1] = 1;
    frame();
    const auto stopped = pose.global[0][3].y;
    weights[1] = 0;
    frame();
    check(std::abs(pose.global[0][3].y - stopped) < TOLERANCE,
        "zero-vector reset morph stops the body without moving it to bind pose");
    config->set_updatable(false);
    weights[0] = 1;
    frame();
    weights[0] = 0;
    config->set_updatable(true);
    frame();
    frame();
    check(std::abs(pose.global[0][3].y - stopped) < TOLERANCE,
        "disabled simulation does not accumulate morph impulses");
}
