#include <assets_system/animation/vmd/vmd_file_struct.h>
#include <component/morph_component.h>
#include <core/system/animation/animation_system.h>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>

using namespace enishi;

namespace {
    void check(bool condition, const char* message) {
        if (!condition) {
            std::cerr << message << '\n';
            std::exit(EXIT_FAILURE);
        }
    }
    template <typename T> void write(std::ofstream& file, const T& value) {
        file.write(reinterpret_cast<const char*>(&value), sizeof(value));
    }
} // namespace

void model_motion_tests() {
    auto registry = std::make_shared<ecs::Registry>();
    const auto entity = registry->create();
    component::ModelComponent model;
    model.bone_names = {"unused", "root"};
    model.morph_targets.targets.emplace_back().name = "smile";
    check(registry->insert(entity, std::move(model)).is_ok(), "register model");
    component::AnimationComponent pose;
    pose.bind_pose.resize(2);
    pose.bind_pose[1].position = {0, 3, 0};
    pose.animation = pose.bind_pose;
    check(registry->insert(entity, std::move(pose)).is_ok(), "register pose");
    check(registry->insert(entity, component::MorphComponent{{0}}).is_ok(), "register morph");
    const auto path = std::filesystem::current_path() / "generated-playback.vmd";
    {
        std::ofstream file(path, std::ios::binary);
        assets_system::VMDHeader header{};
        std::memcpy(
            header.header, "Vocaloid Motion Data 0002", sizeof("Vocaloid Motion Data 0002"));
        write(file, header);
        constexpr std::uint32_t ONE = 1;
        write(file, ONE);
        assets_system::VMDBoneKeyFrame bone{};
        std::memcpy(bone.bone_name, "root", sizeof("root"));
        bone.translation[0] = 1;
        bone.rotation[3] = 1;
        write(file, bone);
        write(file, ONE);
        assets_system::VMDMorphKeyFrame morph{};
        std::memcpy(morph.morph_name, "smile", sizeof("smile"));
        morph.weight = 0.5f;
        write(file, morph);
    }
    core::AnimationSystem system(registry);
    check(system.play_vmd(entity, path, false).is_ok(), "load and play motion file");
    system.update(types::DeltaTime(0.0f));
    check(registry->get<component::AnimationComponent>(entity).unwrap().animation[1].position ==
              glm::vec3(1, 3, 0),
        "motion binds by bone name and adds to rest pose");
    check(registry->get<component::MorphComponent>(entity).unwrap().weights[0] == 0.5f,
        "file morph reaches entity weights");
    std::filesystem::remove(path);
    const auto controller = system.get_controller(entity);
    check(system.play_vmd(entity, path).is_err() && system.get_controller(entity) == controller,
        "load error preserves existing playback");
}
