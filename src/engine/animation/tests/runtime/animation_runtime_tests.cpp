#include <animation/converter/vmd/vmd_converter.h>
#include <animation/keyframe_interpolator.h>
#include <assets_system/model/bone/bone_resolver.h>
#include <cstdlib>
#include <cstring>
#include <iostream>

using namespace enishi;

namespace {
    void check(bool condition, const char* message) {
        if (!condition) {
            std::cerr << message << '\n';
            std::exit(EXIT_FAILURE);
        }
    }

    void bone_tracks_test() {
        assets_system::BoneNameMapConstructor names;
        const std::string full_name = "abcdefghijklmno";
        names.bone_names = {full_name};
        assets_system::BoneResolver resolver(names);
        assets_system::VMDData data{};
        assets_system::VMDBoneKeyFrame first{};
        std::memcpy(first.bone_name, full_name.data(), sizeof(first.bone_name));
        first.rotation[3] = 1;
        auto last = first;
        constexpr std::uint32_t LAST_FRAME = 30;
        last.frame = LAST_FRAME;
        last.translation[0] = 10;
        constexpr std::uint8_t MAX_CONTROL = 127;
        // x(t)=t^3, y(t)=1-(1-t)^3 gives a visibly non-linear destination curve.
        last.interpolation[4] = MAX_CONTROL;
        last.interpolation[12] = MAX_CONTROL;
        auto duplicate = last;
        duplicate.translation[0] = 20;
        data.bone_key_frames = {last, first, duplicate};
        const auto clip = animation::FrameConverter::make_clip_data(&resolver, data);
        check(clip.bone_tracks.size() == 1, "bounded full-width name resolves");
        const auto& track = clip.bone_tracks.front();
        check(
            track.positions.times == std::vector<float>{0, 1}, "keys are sorted and deduplicated");
        check(track.positions.values.back().x == 20, "last duplicate wins");
        const auto midpoint = animation::KeyframeInterpolator::sample(track.positions, 0.5f);
        check(midpoint.x > 18 && midpoint.x < 20, "destination key supplies VMD curve");
        check(clip.duration == 1, "bone duration in seconds");
    }

    void step_boundary_test() {
        animation::Keyframes<float> keys;
        keys.times = {0, 1, 2};
        keys.values = {0, 1, 0};
        keys.interpolation_type = animation::InterpolationType::Step;
        check(animation::KeyframeInterpolator::sample(keys, 1.0f) == 1,
            "step switches at exact key time");
    }
} // namespace

int main() {
    bone_tracks_test();
    step_boundary_test();
    std::cout << "Animation runtime tests passed\n";
}
