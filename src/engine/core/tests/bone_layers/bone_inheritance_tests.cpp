#include <assets_system/model/model_loader/pmx/pmx_to_model_data.h>
#include <core/system/skinning/model_pose_initializer.h>
#include <core/system/skinning/skinning_system.h>
#include <cstdlib>
#include <iostream>
#include <model_controller/model_component_builder.h>

using namespace enishi;

namespace {
    constexpr float TOLERANCE = 0.0001f;
    constexpr std::uint16_t INHERIT = 0x0300;
    constexpr std::uint16_t LOCAL = 0x0080;
    constexpr types::BoneIndex SOURCE = 0;
    constexpr types::BoneIndex DESTINATION = 1;
    constexpr types::BoneIndex FOLLOWER = 2;
    constexpr types::BoneIndex TIP = 3;

    void check(bool condition, const char* message) {
        if (!condition) {
            std::cerr << message << '\n';
            std::exit(EXIT_FAILURE);
        }
    }

    void inheritance_test(bool local, float weight) {
        assets_system::PMXData data;
        data.version = 2.1f;
        data.vertices.emplace_back().bones[0] = SOURCE;
        data.vertices.front().weights[0] = 1;
        data.bones.resize(4);
        data.bones[SOURCE].position = {10, 0, 0};
        auto& destination = data.bones[DESTINATION];
        destination.position = {0, 2, 0};
        destination.flags = INHERIT;
        if (local) {
            destination.flags |= LOCAL;
        }
        destination.inherit_parent = SOURCE;
        destination.inherit_weight = weight;
        destination.layer = 1;
        auto& follower = data.bones[FOLLOWER];
        follower.flags = INHERIT;
        follower.inherit_parent = DESTINATION;
        follower.inherit_weight = 0.5f;
        follower.layer = 2;
        data.bones[TIP].parent = DESTINATION;
        data.bones[TIP].position = {1, 2, 0};
        auto converted = assets_system::PMXToModelData::to_model_data("inherit.pmx", data, nullptr);
        check(converted.is_ok(), "convert inherited bones");
        auto model = model_controller::make_model_component(*converted.unwrap(), {});
        auto registry = std::make_shared<ecs::Registry>();
        const auto entity = registry->create();
        check(registry->insert(entity, model).is_ok(), "insert inheritance model");
        check(core::initialize_model_pose(*registry, entity, model).is_ok(),
            "initialize inheritance pose");
        auto& animation = registry->get<component::AnimationComponent>(entity).unwrap_mut();
        animation.animation[SOURCE].position.x += 4;
        animation.animation[SOURCE].rotation =
            glm::angleAxis(glm::half_pi<float>(), glm::vec3(0, 0, 1));
        animation.animation[DESTINATION].position.y += 1;
        core::SkinningSystem system(registry, nullptr);
        for (int frame = 0; frame < 2; ++frame) {
            system.update(types::DeltaTime(0.0f));
            const auto angle = glm::half_pi<float>() * weight;
            const auto expected = glm::vec3(4 * weight + std::cos(angle), 3 + std::sin(angle), 0);
            check(glm::length(glm::vec3(animation.global[TIP][3]) - expected) < TOLERANCE,
                "inherit rotation and translation without inheriting bind position or drifting");
            check(glm::length(glm::vec3(animation.global[FOLLOWER][3]) -
                              glm::vec3(2 * weight, 0.5f, 0)) < TOLERANCE,
                "multi-level inheritance must include the source's evaluated displacement");
            const auto direction = glm::vec3(animation.global[FOLLOWER] * glm::vec4(1, 0, 0, 0));
            check(glm::length(
                      direction - glm::vec3(std::cos(angle * 0.5f), std::sin(angle * 0.5f), 0)) <
                      TOLERANCE,
                "multi-level inheritance must include evaluated rotation");
        }
    }
    void ik_inheritance_test(std::int32_t layer, glm::vec3 expected) {
        assets_system::PMXData data;
        data.version = 2.1f;
        data.vertices.emplace_back().bones[0] = 0;
        data.vertices.front().weights[0] = 1;
        data.bones.resize(4);
        data.bones[1].parent = 0;
        data.bones[1].position = {1, 0, 0};
        auto& goal = data.bones[2];
        constexpr std::uint16_t IK = 0x0020;
        goal.flags = IK;
        goal.position = {0, 1, 0};
        goal.layer = 1;
        goal.ik_target = 1;
        goal.ik_angle = glm::half_pi<float>();
        goal.ik_iterations = 8;
        goal.ik_links.push_back({0});
        data.bones[3].flags = INHERIT;
        data.bones[3].inherit_parent = 0;
        data.bones[3].inherit_weight = 1;
        data.bones[3].layer = layer;
        auto converted =
            assets_system::PMXToModelData::to_model_data("inherit-ik.pmx", data, nullptr);
        check(converted.is_ok(), "convert IK inheritance");
        auto model = model_controller::make_model_component(*converted.unwrap(), {});
        auto registry = std::make_shared<ecs::Registry>();
        const auto entity = registry->create();
        check(registry->insert(entity, model).is_ok(), "insert IK inheritance model");
        check(core::initialize_model_pose(*registry, entity, model).is_ok(),
            "initialize IK inheritance");
        core::SkinningSystem system(registry, nullptr);
        system.update(types::DeltaTime(0.0f));
        const auto& pose = registry->get<component::AnimationComponent>(entity).unwrap();
        check(glm::length(glm::vec3(pose.global[3] * glm::vec4(1, 0, 0, 0)) - expected) < TOLERANCE,
            "inheritance must observe IK only after its layer has evaluated");
    }
} // namespace

void bone_inheritance_tests() {
    inheritance_test(false, 0.5f);
    inheritance_test(true, 0.5f);
    inheritance_test(false, -0.5f);
    ik_inheritance_test(2, {0, 1, 0});
    ik_inheritance_test(0, {1, 0, 0});
}
