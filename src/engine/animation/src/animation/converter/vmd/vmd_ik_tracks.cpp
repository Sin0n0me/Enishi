#include "vmd_converter.h"
#include <algorithm>
#include <foundation/str/to_utf8.h>
#include <unordered_map>

namespace enishi::animation {
    foundation::VoidResult<AnimationError> FrameConverter::write_ik_track(
        std::vector<IKTrack>& tracks,
        const std::vector<assets_system::VMDIKKeyFrame>& source,
        const assets_system::IBoneResolver* resolver) {
        if (resolver == nullptr) {
            return foundation::Error(AnimationError::FailedConvert, "Bone resolver is missing");
        }
        auto frames = source;
        std::stable_sort(frames.begin(), frames.end(), [](const auto& a, const auto& b) {
            return a.frame < b.frame;
        });
        std::unordered_map<types::BoneIndex, std::size_t> indices;
        for (const auto& frame : frames) {
            for (const auto& info : frame.ik_infos) {
                const std::string name(std::begin(info.name),
                    std::find(std::begin(info.name), std::end(info.name), '\0'));
                const auto decoded = foundation::sjis_to_utf8(name);
                if (decoded.is_err()) {
                    return decoded.propagation(AnimationError::FailedConvert)
                        .add_message(
                            std::format("Failed to decode VMD IK name at frame {}", frame.frame));
                }
                const auto index = resolver->resolve_index(decoded.unwrap());
                if (index.is_none()) {
                    continue;
                }
                const auto [entry, inserted] = indices.try_emplace(index.unwrap(), tracks.size());
                if (inserted) {
                    tracks.emplace_back().bone_index = index.unwrap();
                }
                auto& track = tracks[entry->second];
                const auto time = static_cast<float>(frame.frame) / assets_system::VMD_FPS;
                if (!track.times.empty() && track.times.back() == time) {
                    track.flags.back() = info.flag != 0;
                } else {
                    track.times.push_back(time);
                    track.flags.push_back(info.flag != 0);
                }
            }
        }
        return {};
    }
} // namespace enishi::animation
