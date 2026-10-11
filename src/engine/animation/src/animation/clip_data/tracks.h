#pragma once
#include "interpolation/interpolation.h"
#include "interpolation/interpolation_type.h"
#include <cstdint>
#include <engine_types/assets/model/addons/bone.h>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <vector>

namespace enishi::animation {
    template <typename T> struct Keyframes {
        std::vector<float> times;
        std::vector<T> values;
        InterpolationType interpolation_type = InterpolationType::Linear;
        std::vector<InterpolationData<T>> interpolation;
    };

    struct BoneTrack {
        types::BoneIndex bone_index;
        Keyframes<glm::vec3> positions;
        Keyframes<glm::quat> rotations;
        Keyframes<glm::vec3> scales;
    };

    struct MorphTrack {
        std::uint32_t morph_index;
        Keyframes<float> weights;
    };

    struct IKTrack {
        types::BoneIndex bone_index;
        std::vector<float> times;
        std::vector<bool> flags;
    };
} // namespace enishi::animation
