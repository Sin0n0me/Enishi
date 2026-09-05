#pragma once
#include <engine_types/assets/model/addons/bone.h>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <unordered_map>
#include <vector>

namespace enishi::component {
    using IkIndex = std::uint32_t;

    enum class IKType {
        CCDIK,
        LimitedCCDIK,
    };

    struct IKLimit {
        glm::vec3 axis; // 回転を制限する軸
        float limit;    // 制限する量
    };

    struct CCDIK {
        std::uint32_t iterations;
        types::BoneIndex target;
        types::BoneIndex ik_bone;
        std::vector<types::BoneIndex> chain;
    };

    struct IK {
        IKType ik_type;
        glm::quat rotate;
        IKLimit limit;
        CCDIK ik; // 他のCCDIK以外のIKSolverを作るときはunionで纏めるなどが必要
    };

    struct IKComponent {
        std::unordered_map<types::BoneIndex, IkIndex> ik_map;
        std::vector<IK> iks; // BoneIndex = 配列インデックスではない
    };
} // namespace enishi::component
