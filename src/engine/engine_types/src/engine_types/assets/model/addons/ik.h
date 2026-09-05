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
        LimitedCCDIK,
    };

    struct IKLimit {
        glm::vec3 axis; // 回転を制限する軸
    };

    struct CCDIK {
        std::uint32_t iterations;
        BoneIndex target;
        BoneIndex ik_bone;
        std::vector<BoneIndex> chain;
        float limit; // 制限する量
    };

    struct LimitedCCDIK {
        IKLimit limit;
        CCDIK ccdik;
    };

    using IkMethod = std::variant<CCDIK, LimitedCCDIK>;

    struct IK {
        IkMethod method; // 他のCCDIK以外のIKSolverを作るときはunionで纏めるなどが必要
    };
} // namespace enishi::types