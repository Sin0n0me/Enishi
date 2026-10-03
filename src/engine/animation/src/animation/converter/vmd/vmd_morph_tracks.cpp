#include "vmd_converter.h"
#include <algorithm>
#include <cmath>
#include <foundation/str/to_utf8.h>
#include <unordered_map>

namespace enishi::animation {
    foundation::VoidResult<AnimationError> FrameConverter::write_morph_track(
        std::vector<MorphTrack>& tracks,
        const std::vector<assets_system::VMDMorphKeyFrame>& source,
        const assets_system::IMorphResolver* resolver) {
        auto frames = source;
        std::stable_sort(frames.begin(), frames.end(), [](const auto& a, const auto& b) {
            return a.frame < b.frame;
        });
        std::unordered_map<types::MorphIndex, std::size_t> indices;
        for (const auto& frame : frames) {
            const std::string name(std::begin(frame.morph_name),
                std::find(std::begin(frame.morph_name), std::end(frame.morph_name), '\0'));
            const auto decoded = foundation::sjis_to_utf8(name);
            if (decoded.is_err()) {
                return decoded.propagation(AnimationError::FailedConvert)
                    .add_message(
                        std::format("Failed to decode VMD morph name at frame {}", frame.frame));
            }
            const auto index = resolver->resolve_index(decoded.unwrap());
            if (index.is_none()) {
                continue;
            }
            if (!std::isfinite(frame.weight)) {
                return foundation::Error(AnimationError::FailedConvert,
                    std::format("Non-finite VMD morph weight for {} at frame {}",
                        decoded.unwrap(),
                        frame.frame));
            }
            const auto [entry, inserted] = indices.try_emplace(index.unwrap(), tracks.size());
            if (inserted) {
                tracks.emplace_back().morph_index = index.unwrap();
            }
            auto& track = tracks[entry->second].weights;
            const auto time = static_cast<float>(frame.frame) / assets_system::VMD_FPS;
            if (!track.times.empty() && track.times.back() == time) {
                track.values.back() = frame.weight;
            } else {
                track.times.push_back(time);
                track.values.push_back(frame.weight);
            }
        }
        return {};
    }
} // namespace enishi::animation
