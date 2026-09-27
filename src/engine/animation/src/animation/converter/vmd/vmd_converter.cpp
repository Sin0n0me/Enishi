#include "vmd_converter.h"
#include "../../clip_data/interpolation/interpolation.h"
#include <algorithm>
#include <foundation/log/logger.h>
#include <foundation/str/to_utf8.h>

namespace enishi::animation {
    AnimationClipData FrameConverter::make_clip_data(
        const assets_system::IBoneResolver* resolver, const assets_system::VMDData& data) {
        AnimationClipData clip_data{};

        if (FrameConverter::write_bone_track(clip_data.bone_tracks, data.bone_key_frames, resolver)
                .is_err()) {
            foundation::Logger::warning("Failed to convert VMD bone tracks");
        }

        for (const auto& bone_key_frame : data.bone_key_frames) {
            const float time = static_cast<float>(bone_key_frame.frame) / assets_system::VMD_FPS;
            clip_data.duration = std::max(clip_data.duration, time);
        }

        /*
        if (FrameConverter::write_morph_track(clip_data.morph_tracks, data.morph_key_frames)
                .is_err()) {
        }
        */

        return clip_data;
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

    foundation::VoidResult<AnimationError> FrameConverter::write_morph_track(
        std::vector<MorphTrack>& morph_tracks,
        const std::vector<assets_system::VMDMorphKeyFrame>& morph_key_frames,
        const assets_system::IMorphResolver* resolver) {
        std::unordered_map<std::uint32_t, std::uint32_t> tmep;

        for (const auto& key_frames : morph_key_frames) {
            const auto utf8 = foundation::sjis_to_utf8(key_frames.morph_name);
            if (utf8.is_err()) {
                foundation::Logger::warning("UTF8に変換できない文字が含まれています");
                continue;
            }

            const auto opt_index = resolver->resolve_index(utf8.unwrap());
            if (opt_index.is_none()) {
                foundation::Logger::warning("モデルに存在しないボーンが含まれています");
                continue;
            };
            const auto index = opt_index.unwrap();

            // 0はすべてのベースなので除外
            if (index == 0) {
                continue;
            }

            // 意図したものかはわからないので警告は出す
            if (key_frames.weight < 0) {
                foundation::Logger::warning("モーフに符号がマイナスのウエイトがあります");
            }

            if (!tmep.contains(index)) {
                const auto track_index = morph_tracks.size() - 1;
                tmep[index] = track_index;

                // 初回追加時のみ
                if (track_index == 0) {
                    MorphTrack& bone_track = morph_tracks[0];
                    bone_track.morph_index = index;

                    // モーフは通常の線形補間
                    bone_track.weights.interpolation_type = InterpolationType::Linear;
                }
            }

            MorphTrack& morph_track = morph_tracks[tmep[index]];

            // フレーム単位なので時間に変換
            const float time = static_cast<float>(key_frames.frame) / assets_system::VMD_FPS;
            morph_track.weights.times.emplace_back(time);

            morph_track.weights.values.emplace_back(key_frames.weight);
        }

        return {};
    }
} // namespace enishi::animation
