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

    struct IKLinkLimit {
        bool enabled{};
        glm::vec3 lower{}; // Local Euler angles in radians, aligned with CCDIK::chain.
        glm::vec3 upper{};
    };

    struct CCDIK {
        std::uint32_t iterations;
        BoneIndex target;
        BoneIndex ik_bone;
        std::vector<BoneIndex> chain;
        IkLimit limit;
        std::vector<IKLinkLimit> link_limits;
    };

    using IkMethod = std::variant<CCDIK>;

    struct IK {
        IkMethod method;
    };
} // namespace enishi::types
