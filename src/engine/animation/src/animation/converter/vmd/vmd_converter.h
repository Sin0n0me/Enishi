#pragma once
#include "../../clip_data/animation.h"
#include "../../errors/errors.h"
#include <assets_system/animation/vmd/vmd_data.h>
#include <assets_system/model/interface_bone_resolver.h>
#include <assets_system/model/interface_morph_resolver.h>
#include <foundation/result/result.h>

namespace enishi::animation {
    //
    class FrameConverter {
      public:
        static AnimationClipData make_clip_data(
            const assets_system::IBoneResolver* resolver, const assets_system::VMDData& data);

      private:
        static foundation::VoidResult<AnimationError> write_bone_track(
            std::vector<BoneTrack>& bone_tracks,
            const std::vector<assets_system::VMDBoneKeyFrame>& bone_key_frames,
            const assets_system::IBoneResolver* resolver);

        static foundation::VoidResult<AnimationError> write_morph_track(
            std::vector<MorphTrack>& morph_tracks,
            const std::vector<assets_system::VMDMorphKeyFrame>& morph_key_frames,
            const assets_system::IMorphResolver* resolver);

        static foundation::VoidResult<AnimationError> write_ik_track(
            std::vector<IKTrack>& ik_tracks,
            const std::vector<assets_system::VMDIKKeyFrame>& ik_key_frames,
            const assets_system::IMorphResolver* resolver);
    };
} // namespace enishi::animation
