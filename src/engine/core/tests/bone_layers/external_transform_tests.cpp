#include <assets_system/model/model_loader/pmx/pmx_to_model_data.h>
#include <core/system/skinning/model_pose_initializer.h>
#include <core/system/skinning/skinning_system.h>
#include <cstdlib>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <model_controller/model_component_builder.h>

using namespace enishi;
namespace {
    void check(bool condition, const char* message) {
        if (!condition) {
            std::cerr << message << '\n';
            std::exit(EXIT_FAILURE);
        }
    }
} // namespace

void external_transform_tests() {
    constexpr std::uint16_t EXTERNAL_PARENT = 0x2000;
    constexpr std::int32_t SLOT = 42;
    constexpr float TOLERANCE = 0.0001f;
    assets_system::PMXData data;
    data.version = 2.0f;
    data.vertices.emplace_back().bones[0] = 0;
    data.vertices.front().weights[0] = 1;
    data.bones.resize(3);
    data.bones[1].parent = 0;
    data.bones[1].position = {1, 0, 0};
    data.bones[1].flags = EXTERNAL_PARENT;
    data.bones[1].external_parent = SLOT;
    data.bones[2].parent = 1;
    data.bones[2].position = {2, 0, 0};
    const auto converted =
        assets_system::PMXToModelData::to_model_data("external.pmx", data, nullptr);
    check(converted.is_ok(), "convert external parent slot");
    auto model = model_controller::make_model_component(*converted.unwrap(), {});
    check(model.bone_constraints.constraints[1].external_transform_slot == SLOT,
        "external key becomes format-neutral transform slot");
    auto registry = std::make_shared<ecs::Registry>();
    const auto entity = registry->create();
    check(registry->insert(entity, model).is_ok(), "insert external model");
    check(
        core::initialize_model_pose(*registry, entity, model).is_ok(), "initialize external pose");
    auto& pose = registry->get<component::AnimationComponent>(entity).unwrap_mut();
    auto& bindings =
        registry->get<component::ModelComponent>(entity).unwrap_mut().external_transforms;
    pose.animation[0].position = {1, 0, 0};
    core::SkinningSystem system(registry, nullptr);
    system.update(types::DeltaTime(0.0f));
    check(glm::vec3(pose.global[2][3]) == glm::vec3(3, 0, 0), "unbound slot is identity");
    bindings[SLOT] = glm::translate(glm::mat4(1), glm::vec3(0, 2, 0)) *
                     glm::rotate(glm::mat4(1), glm::half_pi<float>(), glm::vec3(0, 0, 1));
    for (int frame = 0; frame < 2; ++frame) {
        system.update(types::DeltaTime(0.0f));
        check(glm::length(glm::vec3(pose.global[2][3]) - glm::vec3(0, 5, 0)) < TOLERANCE,
            "external transform composes after parent and propagates once to descendants");
        check(glm::vec3(pose.global[0][3]) == glm::vec3(1, 0, 0), "parent is unaffected");
    }
    bindings.clear();
    system.update(types::DeltaTime(0.0f));
    check(glm::vec3(pose.global[2][3]) == glm::vec3(3, 0, 0), "unbinding restores animation");
}
