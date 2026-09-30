#include "clip_sampler.h"
#include <algorithm>
#include <animation/keyframe_interpolator.h>

namespace enishi::core {
    namespace {
        template <typename T> bool has_keys(const animation::Keyframes<T>& keys) {
            return !keys.times.empty() && keys.times.size() == keys.values.size();
        }

        void sample_bones(const animation::AnimationClipData& clip,
            float time,
            component::AnimationComponent& pose) {
            for (std::size_t index = 0; index < pose.animation.size(); ++index) {
                pose.animation[index] = index < pose.bind_pose.size()
                                            ? pose.bind_pose[index]
                                            : component::AnimationBuffer{};
            }
            for (const auto& track : clip.bone_tracks) {
                if (!(track.bone_index < pose.animation.size())) {
                    continue;
                }
                auto& bone = pose.animation[track.bone_index];
                const auto bind = bone;
                if (has_keys(track.positions)) {
                    bone.position = animation::KeyframeInterpolator::sample(track.positions, time);
                    if (clip.relative_to_bind_pose) {
                        bone.position += bind.position;
                    }
                }
                if (has_keys(track.rotations)) {
                    bone.rotation = animation::KeyframeInterpolator::sample(track.rotations, time);
                    if (clip.relative_to_bind_pose) {
                        bone.rotation = bind.rotation * bone.rotation;
                    }
                }
                if (has_keys(track.scales)) {
                    bone.scale = animation::KeyframeInterpolator::sample(track.scales, time);
                    if (clip.relative_to_bind_pose) {
                        bone.scale *= bind.scale;
                    }
                }
            }
        }

        void sample_morphs(const animation::AnimationClipData& clip,
            float time,
            component::MorphComponent& morph) {
            std::fill(morph.weights.begin(), morph.weights.end(), 0.0f);
            for (const auto& track : clip.morph_tracks) {
                if (track.morph_index < morph.weights.size() && has_keys(track.weights)) {
                    morph.weights[track.morph_index] =
                        animation::KeyframeInterpolator::sample(track.weights, time);
                }
            }
        }

        void sample_ik(
            const animation::AnimationClipData& clip, float time, component::IKComponent& ik) {
            ik.disabled_bones.clear();
            for (const auto& track : clip.ik_tracks) {
                if (track.times.size() != track.flags.size()) {
                    continue;
                }
                const auto next = std::upper_bound(track.times.begin(), track.times.end(), time);
                if (next == track.times.begin()) {
                    continue;
                }
                const auto index = static_cast<std::size_t>(next - track.times.begin() - 1);
                if (!track.flags[index]) {
                    ik.disabled_bones.insert(track.bone_index);
                }
            }
        }
    } // namespace

    void sample_model_clip(const animation::AnimationClipData& clip,
        float time,
        component::AnimationComponent& pose,
        component::MorphComponent* morph,
        component::IKComponent* ik) {
        sample_bones(clip, time, pose);
        if (morph != nullptr) {
            sample_morphs(clip, time, *morph);
        }
        if (ik != nullptr) {
            sample_ik(clip, time, *ik);
        }
        pose.elapsed_time = time;
    }
} // namespace enishi::core
