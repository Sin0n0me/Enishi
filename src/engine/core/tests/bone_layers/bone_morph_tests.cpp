#include <assets_system/model/model_loader/pmx/pmx_to_model_data.h>
#include <component/morph_component.h>
#include <core/system/skinning/model_pose_initializer.h>
#include <core/system/skinning/skinning_system.h>
#include <cstdlib>
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

void bone_morph_tests() {
    constexpr float tolerance = 0.0001f;
    assets_system::PMXData data;
    data.version = 2.1f;
    data.vertices.emplace_back().bones[0] = 0;
    data.vertices.front().weights[0] = 1;
    data.bones.resize(2);
    data.bones[1].parent = 0;
    data.bones[1].position = {1, 0, 0};
    data.morphs.resize(2);
    data.morphs[0].type = assets_system::PMXMorphType::Bone;
    auto& offset = data.morphs[0].offsets.emplace_back();
    offset.index = 0;
    offset.translation = {0, 1, 0};
    const auto rotation = glm::angleAxis(glm::half_pi<float>(), glm::vec3(0, 0, 1));
    offset.rotation = {rotation.x, rotation.y, rotation.z, rotation.w};
    auto& group = data.morphs[1].offsets.emplace_back();
    group.index = 0;
    group.weight = 0.5f;
    auto converted = assets_system::PMXToModelData::to_model_data("bone-morph.pmx", data, nullptr);
    check(converted.is_ok(), "convert bone and group morphs");
    auto model = model_controller::make_model_component(*converted.unwrap(), {});
    auto registry = std::make_shared<ecs::Registry>();
    const auto entity = registry->create();
    check(registry->insert(entity, model).is_ok(), "insert bone morph model");
    check(core::initialize_model_pose(*registry, entity, model).is_ok(),
        "initialize bone morph pose");
    auto& pose = registry->get<component::AnimationComponent>(entity).unwrap_mut();
    pose.animation[0].position = {1, 0, 0};
    pose.animation[0].rotation = glm::angleAxis(-glm::quarter_pi<float>(), glm::vec3(0, 0, 1));
    auto& weights = registry->get<component::MorphComponent>(entity).unwrap_mut().weights;
    weights[1] = 1;
    core::SkinningSystem system(registry, nullptr);
    for (int frame = 0; frame < 2; ++frame) {
        system.update(types::DeltaTime(0.0f));
        check(glm::length(glm::vec3(pose.global[1][3]) - glm::vec3(2, 0.5f, 0)) < tolerance,
            "bone morph composes on animation without accumulating");
        check(pose.animation[0].position == glm::vec3(1, 0, 0),
            "authored animation must remain unchanged");
    }
    weights[1] = 0;
    system.update(types::DeltaTime(0.0f));
    const auto diagonal = std::sqrt(0.5f);
    check(glm::length(glm::vec3(pose.global[1][3]) - glm::vec3(1 + diagonal, -diagonal, 0)) <
              tolerance,
        "zero weight restores the unmodified animation");
}
