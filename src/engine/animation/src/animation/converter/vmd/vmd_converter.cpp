#include "vmd_converter.h"
#include "../../clip_data/interpolation/interpolation.h"
#include <algorithm>
#include <foundation/log/logger.h>
#include <foundation/str/to_utf8.h>

namespace enishi::animation {
    AnimationClipData FrameConverter::make_clip_data(const assets_system::IBoneResolver* resolver,
        const assets_system::VMDData& data,
        const assets_system::IMorphResolver* morph_resolver) {
        auto result = FrameConverter::convert_clip_data(resolver, morph_resolver, data);
        if (result.is_err()) {
            foundation::Logger::warning(
                "Failed to convert VMD animation: " + result.unwrap_err().get_message());
            return {};
        }
        return std::move(result).unwrap();
    }

    foundation::Result<AnimationClipData, AnimationError> FrameConverter::convert_clip_data(
        const assets_system::IBoneResolver* bones,
        const assets_system::IMorphResolver* morphs,
        const assets_system::VMDData& data) {
        AnimationClipData clip{};
        clip.relative_to_bind_pose = true;
        auto result =
            FrameConverter::write_bone_track(clip.bone_tracks, data.bone_key_frames, bones);
        if (result.is_err()) {
            return result.propagation(AnimationError::FailedConvert);
        }
        if (morphs != nullptr) {
            result =
                FrameConverter::write_morph_track(clip.morph_tracks, data.morph_key_frames, morphs);
            if (result.is_err()) {
                return result.propagation(AnimationError::FailedConvert);
            }
        }
        result = FrameConverter::write_ik_track(clip.ik_tracks, data.iks, bones);
        if (result.is_err()) {
            return result.propagation(AnimationError::FailedConvert);
        }
        for (const auto& track : clip.bone_tracks) {
            clip.duration = std::max(clip.duration, track.positions.times.back());
        }
        for (const auto& track : clip.morph_tracks) {
            clip.duration = std::max(clip.duration, track.weights.times.back());
        }
        for (const auto& track : clip.ik_tracks) {
            clip.duration = std::max(clip.duration, track.times.back());
        }
        return clip;
    }

    foundation::VoidResult<AnimationError> FrameConverter::write_bone_track(
        std::vector<BoneTrack>& bone_tracks,
        const std::vector<assets_system::VMDBoneKeyFrame>& bone_key_frames,
        const assets_system::IBoneResolver* resolver) {
        if (resolver == nullptr) {
            return foundation::Error(AnimationError::FailedConvert, "Bone resolver is missing");
        }
        auto frames = bone_key_frames;
        std::stable_sort(frames.begin(), frames.end(), [](const auto& a, const auto& b) {
            return a.frame < b.frame;
        });
        std::unordered_map<types::BoneIndex, std::size_t> tracks;
        for (const auto& frame : frames) {
            const std::string name(std::begin(frame.bone_name),
                std::find(std::begin(frame.bone_name), std::end(frame.bone_name), '\0'));
            const auto utf8 = foundation::sjis_to_utf8(name);
            if (utf8.is_err()) {
                return utf8.propagation(AnimationError::FailedConvert);
            }
            const auto index = resolver->resolve_index(utf8.unwrap());
            if (index.is_none()) {
                continue;
            }
            const auto [entry, inserted] = tracks.try_emplace(index.unwrap(), bone_tracks.size());
            if (inserted) {
                auto& track = bone_tracks.emplace_back();
                track.bone_index = index.unwrap();
                track.positions.interpolation_type = InterpolationType::VmdBezier;
                track.rotations.interpolation_type = InterpolationType::VmdBezier;
            }
            auto& track = bone_tracks[entry->second];
            const auto time = static_cast<float>(frame.frame) / assets_system::VMD_FPS;
            // Last source key wins at a duplicate frame, including its interpolation curve.
            if (!track.positions.times.empty() && track.positions.times.back() == time) {
                track.positions.times.pop_back();
                track.positions.values.pop_back();
                track.positions.interpolation.pop_back();
                track.rotations.times.pop_back();
                track.rotations.values.pop_back();
                track.rotations.interpolation.pop_back();
            }
            const auto curve = VMDAnimationBezier::make(std::to_array(frame.interpolation));
            track.positions.times.push_back(time);
            track.rotations.times.push_back(time);
            track.positions.values.emplace_back(
                frame.translation[0], frame.translation[1], frame.translation[2]);
            const glm::quat rotation(
                frame.rotation[3], frame.rotation[0], frame.rotation[1], frame.rotation[2]);
            track.rotations.values.push_back(
                glm::length(rotation) > 0.0f ? glm::normalize(rotation) : glm::quat(1, 0, 0, 0));
            track.positions.interpolation.emplace_back(curve);
            track.rotations.interpolation.emplace_back(curve);
        }

        return {};
    }

} // namespace enishi::animation
