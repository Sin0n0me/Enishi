#pragma once
#include "interpolation/interpolation.h"
#include "tracks.h"
#include <engine_types/assets/model/addons/bone.h>
#include <string>
#include <vector>

namespace enishi::animation {
    struct AnimationClipData {
        std::string name;
        std::vector<BoneTrack> bone_tracks;
        std::vector<MorphTrack> morph_tracks;
        std::vector<IKTrack> ik_tracks;
        float duration; // 秒
        bool is_looping;
    };
} // namespace enishi::animation