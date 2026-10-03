#include <assets_system/model/model_loader/pmx/pmx_to_model_data.h>
#include <core/system/render/bone_matrix_upload.h>
#include <core/system/skinning/model_pose_initializer.h>
#include <core/system/skinning/skinning_system.h>
#include <cstdlib>
#include <cstring>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <limits>
#include <model_controller/model_component_builder.h>

using namespace enishi;

namespace {
    constexpr types::BoneIndex ROOT = 0;
    constexpr types::BoneIndex TIP = 1;
    constexpr types::BoneIndex GOAL_A = 2;
    constexpr types::BoneIndex GOAL_B = 3;
    constexpr std::size_t BONE_COUNT = 4;
    constexpr std::uint16_t BONE_IK = 0x0020;
    constexpr float TOLERANCE = 0.0001f;

    void check(bool value, const char* message) {
        if (!value) {
            std::cerr << message << '\n';
            std::exit(EXIT_FAILURE);
        }
    }

    assets_system::PMXData fixture(std::int32_t layer_a, std::int32_t layer_b) {
        assets_system::PMXData data;
        data.version = 2.1f;
        data.vertices.emplace_back().bones[0] = ROOT;
        data.vertices.front().weights[0] = 1;
        data.bones.resize(BONE_COUNT);
        data.bones[TIP].parent = ROOT;
        data.bones[TIP].position = {1, 0, 0};
        data.bones[GOAL_A].position = {0, 1, 0};
        data.bones[GOAL_B].position = {0, -1, 0};
        data.bones[GOAL_A].layer = layer_a;
        data.bones[GOAL_B].layer = layer_b;
        for (const auto index : {GOAL_A, GOAL_B}) {
            auto& bone = data.bones[index];
            bone.flags = BONE_IK;
            bone.ik_target = TIP;
            bone.ik_iterations = 8;
            bone.ik_angle = glm::half_pi<float>();
            bone.ik_links.push_back({ROOT});
        }
        return data;
    }

    class BufferUpdater final : public platform::IResourceUpdater {
      public:
        types::OwnedRenderData data;
        bool uploaded = false;
        explicit BufferUpdater(std::size_t count)
            : data(std::vector<glm::mat4>(count, glm::mat4(1))) {
        }
        void on_update() override {
            this->uploaded = true;
        }
        types::OwnedRenderData& get_resource() override {
            return this->data;
        }
    };

    void runtime_test(assets_system::PMXData data, glm::vec3 expected, types::BoneIndex tip = TIP) {
        auto converted = assets_system::PMXToModelData::to_model_data("layers.pmx", data, nullptr);
        check(converted.is_ok(), "layered PMX conversion must succeed");
        auto model = model_controller::make_model_component(*converted.unwrap(), {});
        check(model.bone_constraints.constraints.size() == data.bones.size(),
            "bone manipulation metadata must survive model creation");
        check(model.bone_node[tip].parent == static_cast<types::BoneIndex>(data.bones[tip].parent),
            "evaluation sorting must preserve bone references");
        auto registry = std::make_shared<ecs::Registry>();
        const auto entity = registry->create();
        check(registry->insert(entity, model).is_ok(), "insert model");
        check(!registry->has<component::AnimationComponent>(entity), "distinct component types");
        check(core::initialize_model_pose(*registry, entity, model).is_ok(), "initialize pose");
        core::SkinningSystem system(registry, nullptr);
        system.update(types::DeltaTime(0.0f));
        const auto& pose = registry->get<component::AnimationComponent>(entity).unwrap();
        if (!(glm::length(glm::vec3(pose.global[tip][3]) - expected) < TOLERANCE)) {
            const auto actual = glm::vec3(pose.global[tip][3]);
            std::cerr << "Expected " << expected.x << ',' << expected.y << ',' << expected.z
                      << " got " << actual.x << ',' << actual.y << ',' << actual.z << '\n';
        }
        check(glm::length(glm::vec3(pose.global[tip][3]) - expected) < TOLERANCE,
            "IK goals sharing a joint must execute in layer order");
        const auto& skinning = registry->get<component::SkinningComponent>(entity).unwrap();
        const auto position = glm::vec3(model.bind_bone[tip].global[3]);
        check(glm::length(glm::vec3(skinning.skinning_matrices[tip] * glm::vec4(position, 1)) -
                          expected) < TOLERANCE,
            "IK result must reach final skinning matrices");
        BufferUpdater updater(BONE_COUNT);
        check(core::write_bone_matrices(updater, skinning.skinning_matrices).is_ok() &&
                  updater.uploaded,
            "final matrices must reach the renderer updater");
        glm::mat4 uploaded;
        std::memcpy(&uploaded,
            updater.data.get_render_data().raw_data() + tip * sizeof(glm::mat4),
            sizeof(uploaded));
        check(uploaded == skinning.skinning_matrices[tip], "uploaded matrix contents");
        system.update(types::DeltaTime(0.0f));
        check(glm::length(glm::vec3(pose.global[tip][3]) - expected) < TOLERANCE,
            "IK corrections must not drift across frames");
        BufferUpdater too_small(1);
        check(core::write_bone_matrices(too_small, skinning.skinning_matrices).is_err() &&
                  !too_small.uploaded,
            "undersized GPU buffer must not be uploaded");
        auto ik = registry->get<component::IKComponent>(entity);
        ik.unwrap_mut().disabled_bones = {GOAL_A, GOAL_B};
        system.update(types::DeltaTime(0.0f));
        check(glm::length(glm::vec3(pose.global[tip][3]) - position) < TOLERANCE,
            "disabled IK chains must preserve the animation pose");
    }

    void stable_order_test() {
        const auto source = fixture(10, 10);
        auto converted = assets_system::PMXToModelData::to_model_data("ties.pmx", source, nullptr);
        check(converted.is_ok(), "same-layer conversion");
        auto model = model_controller::make_model_component(*converted.unwrap(), {});
        check(model.evaluation_order == std::vector<types::BoneIndex>{ROOT, TIP, GOAL_A, GOAL_B},
            "same layer uses original bone order");
    }

    void no_ik_test() {
        auto source = fixture(10, 20);
        for (auto& bone : source.bones) {
            bone.flags = 0;
            bone.ik_links.clear();
        }
        auto converted = assets_system::PMXToModelData::to_model_data("no-ik.pmx", source, nullptr);
        check(converted.is_ok(), "layers without IK are supported");
        auto model = model_controller::make_model_component(*converted.unwrap(), {});
        auto registry = std::make_shared<ecs::Registry>();
        const auto entity = registry->create();
        check(registry->insert(entity, model).is_ok(), "insert no-IK model");
        check(
            core::initialize_model_pose(*registry, entity, model).is_ok(), "initialize no-IK pose");
        check(!registry->has<component::IKComponent>(entity), "no synthetic IK component");
        core::SkinningSystem system(registry, nullptr);
        system.update(types::DeltaTime(0.0f));
        const auto& matrices =
            registry->get<component::SkinningComponent>(entity).unwrap().skinning_matrices;
        for (const auto& matrix : matrices) {
            check(matrix == glm::mat4(1), "bind pose must remain unchanged");
        }
    }

    void mixed_entities_test() {
        auto registry = std::make_shared<ecs::Registry>();
        const auto add_partial = [&]() {
            const auto entity = registry->create();
            check(registry->insert(entity, component::AnimationComponent{}).is_ok(),
                "insert partial pose");
        };
        const auto add_model = [&]() {
            auto converted =
                assets_system::PMXToModelData::to_model_data("mixed.pmx", fixture(20, 10), nullptr);
            check(converted.is_ok(), "mixed model conversion");
            auto model = model_controller::make_model_component(*converted.unwrap(), {});
            const auto entity = registry->create();
            check(registry->insert(entity, model).is_ok(), "insert mixed model");
            check(core::initialize_model_pose(*registry, entity, model).is_ok(),
                "initialize mixed pose");
            return entity;
        };
        add_partial();
        const auto first = add_model();
        add_partial();
        const auto second = add_model();
        add_partial();
        core::SkinningSystem system(registry, nullptr);
        system.update(types::DeltaTime(0.0f));
        std::size_t count = 0;
        for (auto [entity, pose, model, skinning] : registry->view<component::AnimationComponent,
                                                    component::ModelComponent,
                                                    component::SkinningComponent>()) {
            check(entity == first || entity == second, "view must skip incomplete entities");
            check(glm::length(glm::vec3(pose.global[TIP][3]) - glm::vec3(0, 1, 0)) < TOLERANCE,
                "all complete models must be evaluated");
            ++count;
        }
        constexpr std::size_t COMPLETE_MODELS = 2;
        check(count == COMPLETE_MODELS, "view must visit each complete model once");
    }
} // namespace

void bone_inheritance_tests();
void bone_morph_tests();

int main() {
    bone_inheritance_tests();
    bone_morph_tests();
    extern void external_transform_tests();
    external_transform_tests();
    auto limited = fixture(20, 10);
    limited.bones[GOAL_A].ik_links.front().limited = 1;
    limited.bones[GOAL_A].ik_links.front().lower = {0, 0, 0};
    limited.bones[GOAL_A].ik_links.front().upper = {0, 0, 0};
    runtime_test(limited, {1, 0, 0});
    auto axes = fixture(20, 10);
    constexpr std::uint16_t manipulation_axes = 0x0c00;
    axes.bones[ROOT].flags |= manipulation_axes;
    axes.bones[ROOT].fixed_axis = {1, 0, 0};
    axes.bones[ROOT].local_x = {1, 0, 0};
    axes.bones[ROOT].local_z = {0, 0, 1};
    runtime_test(axes, {0, 1, 0});
    runtime_test(fixture(10, 20), {0, -1, 0});
    runtime_test(fixture(20, 10), {0, 1, 0});
    runtime_test(fixture(10, 10), {0, -1, 0});
    auto after_physics = fixture(-10, 20);
    constexpr std::uint16_t AFTER_PHYSICS = 0x1000;
    after_physics.bones[GOAL_A].flags |= AFTER_PHYSICS;
    runtime_test(after_physics, {0, 1, 0});
    after_physics.bones[GOAL_B].flags |= AFTER_PHYSICS;
    runtime_test(after_physics, {0, -1, 0});
    runtime_test(
        fixture(std::numeric_limits<std::int32_t>::max(), std::numeric_limits<std::int32_t>::min()),
        {0, 1, 0});
    auto forward = fixture(20, 10);
    forward.bones[ROOT].parent = TIP;
    forward.bones[ROOT].position = {1, 0, 0};
    forward.bones[TIP].parent = -1;
    forward.bones[TIP].position = {0, 0, 0};
    for (const auto goal : {GOAL_A, GOAL_B}) {
        forward.bones[goal].ik_target = ROOT;
        forward.bones[goal].ik_links.front().bone = TIP;
    }
    runtime_test(forward, {0, 1, 0}, ROOT);
    stable_order_test();
    no_ik_test();
    mixed_entities_test();
    std::cout << "Bone layer integration tests passed\n";
}
