#pragma once
#include "interpolation/interpolation.h"
#include "interpolation/interpolation_type.h"
#include <cstdint>
#include <engine_types/assets/model/addons/bone.h>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <vector>

namespace enishi::animation {
    // 1ボーンの1軸分のキーフレーム列
    template <typename T> struct Keyframes {
        std::vector<float> times;             // 秒単位
        std::vector<T> values;                // 各フレーム毎の値
        InterpolationType interpolation_type; // 補間方式
        InterpolationData<T> interpolation;   // 補間に使用するデータ
    };

    struct BoneTrack {
        types::BoneIndex bone_index;
        Keyframes<glm::vec3> positions;
        Keyframes<glm::quat> rotations;
        Keyframes<glm::vec3> scales;
    };

    // モーフターゲットのトラック
    struct MorphTrack {
        std::uint32_t morph_index; // MorphTarget の添字
        Keyframes<float> weights;  // 0.0 〜 1.0
    };

    struct IKTrack {
        types::BoneIndex bone_index;
        std::vector<float> times; // 秒単位
        std::vector<bool> flags;  // オンオフフラグ
    };
} // namespace enishi::animation