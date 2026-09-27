#include <animation/converter/vmd/vmd_converter.h>
#include <animation/keyframe_interpolator.h>
#include <assets_system/model/bone/bone_resolver.h>
#include <assets_system/model/morph/morph_resolver.h>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <limits>

using namespace enishi;

namespace {
    void check(bool condition, const char* message) {
        if (!condition) {
            std::cerr << message << '\n';
            std::exit(EXIT_FAILURE);
        }
    }
} // namespace

void vmd_expression_tests() {
    assets_system::BoneNameMapConstructor bone_names;
    bone_names.bone_names = {"leg"};
    assets_system::BoneResolver bones(bone_names);
    assets_system::MorphNameMapConstructor morph_names;
    morph_names.morph_names = {"smile"};
    assets_system::MorphResolver morphs(morph_names);
    assets_system::VMDData source{};
    assets_system::VMDMorphKeyFrame first{};
    std::memcpy(first.morph_name, "smile", sizeof("smile"));
    auto last = first;
    last.frame = 60;
    last.weight = 1;
    source.morph_key_frames = {last, first};
    assets_system::VMDIKKeyFrame ik{};
    ik.frame = 90;
    ik.ik_infos.emplace_back();
    std::memcpy(ik.ik_infos[0].name, "leg", sizeof("leg"));
    source.iks.push_back(ik);
    auto clip = animation::FrameConverter::convert_clip_data(&bones, &morphs, source);
    check(clip.is_ok(), "convert morph-only motion with IK switches");
    const auto& value = clip.unwrap();
    check(value.relative_to_bind_pose && value.duration == 3,
        "duration includes IK and morph tracks");
    check(value.morph_tracks.size() == 1 && value.morph_tracks[0].morph_index == 0,
        "first model morph is animated");
    check(animation::KeyframeInterpolator::sample(value.morph_tracks[0].weights, 1.0f) == 0.5f,
        "morph weight interpolation");
    check(value.ik_tracks.size() == 1 && value.ik_tracks[0].bone_index == 0 &&
              !value.ik_tracks[0].flags[0],
        "IK state is bound to controller bone");
    source.morph_key_frames[0].weight = std::numeric_limits<float>::quiet_NaN();
    check(animation::FrameConverter::convert_clip_data(&bones, &morphs, source).is_err(),
        "invalid weight is propagated");
}
