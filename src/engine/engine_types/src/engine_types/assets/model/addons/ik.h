#pragma once
#include "bone.h"
#include <cstdint>
#include <glm/glm.hpp>
#include <variant>
#include <vector>

namespace enishi::types {
    using IkIndex = std::uint32_t;

    enum class IKType {
        CCDIK,
    };

    struct IKLimitAngle {
        float limit; // 制限する量
    };

    struct IKLimitAxis {
        glm::vec3 axis; // 回転を制限する軸
        float limit;    // 制限する量
    };

    using IkLimit = std::variant<IKLimitAngle, IKLimitAxis>;

    struct CCDIK {
        std::uint32_t iterations;
        BoneIndex target;
        BoneIndex ik_bone;
        std::vector<BoneIndex> chain;
        IkLimit limit;
    };

    using IkMethod = std::variant<CCDIK>;

    struct IK {
        IkMethod method;
    };
} // namespace enishi::types