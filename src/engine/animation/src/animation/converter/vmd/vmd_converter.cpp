#include "vmd_converter.h"
#include "../../clip_data/interpolation/interpolation.h"
#include <foundation/log/logger.h>
#include <foundation/str/to_utf8.h>

namespace enishi::animation {
    AnimationClipData FrameConverter::make_clip_data(
        const assets_system::IBoneResolver* resolver, const assets_system::VMDData& data) {
        AnimationClipData clip_data{};

        if (FrameConverter::write_bone_track(clip_data.bone_tracks, data.bone_key_frames, resolver)
                .is_err()) {
            // return;
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
        std::unordered_map<std::uint32_t, std::uint32_t> tmep;

        for (const auto& bone_key_frame : bone_key_frames) {
            const auto utf8 = foundation::sjis_to_utf8(bone_key_frame.bone_name);
            if (utf8.is_err()) {
                foundation::Logger::warning("UTF8に変換できない文字が含まれています");
                continue;
            }

            const auto opt_index = resolver->resolve_index(utf8.unwrap());
            if (opt_index.is_none()) {
                foundation::Logger::warning("モデルに存在しないボーンが含まれています");
                continue;
            };
            const auto bone_index = opt_index.value();

            if (!tmep.contains(bone_index)) {
                const auto track_index = bone_tracks.size() - 1;
                tmep[bone_index] = track_index;

                // 初回追加時のみ
                if (track_index == 0) {
                    BoneTrack& bone_track = bone_tracks[0];
                    bone_track.bone_index = bone_index;
                    bone_track.positions.interpolation_type = InterpolationType::VmdBezier;
                    bone_track.rotations.interpolation_type = InterpolationType::VmdBezier;
                }
            }

            BoneTrack& bone_track = bone_tracks[tmep[bone_index]];

            // フレーム単位なので時間に変換
            const float time = static_cast<float>(bone_key_frame.frame) / assets_system::VMD_FPS;
            bone_track.positions.times.emplace_back(time);
            bone_track.rotations.times.emplace_back(time);

            // アニメーションの補完用
            bone_track.scales;
            const auto bezier =
                VMDAnimationBezier::make(std::to_array(bone_key_frame.interpolation));

            const auto translate = glm::vec3(bone_key_frame.translation[0],
                bone_key_frame.translation[1],
                bone_key_frame.translation[2]);

            // VMDはxyzwの順序でデータが格納されている
            const auto rotate = glm::quat(bone_key_frame.rotation[3],
                bone_key_frame.rotation[0],
                bone_key_frame.rotation[1],
                bone_key_frame.rotation[2]);

            bone_track.positions.values.emplace_back(translate);
            bone_track.rotations.values.emplace_back(rotate);
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
            const auto index = opt_index.value();

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