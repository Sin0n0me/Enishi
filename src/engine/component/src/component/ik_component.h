#pragma once
#include <engine_types/assets/model/addons/bone.h>
#include <engine_types/assets/model/addons/ik.h>
#include <glm/glm.hpp>
#include <unordered_map>
#include <vector>

namespace enishi::component {
    struct IKComponent {
        std::unordered_map<types::BoneIndex, types::IkIndex> ik_map;
        std::vector<types::IK> iks;      // 配列インデックス = IkIndex
        std::vector<glm::quat> rotation; // 配列インデックス = BoneIndex
        std::vector<glm::mat4> globals;  // 配列インデックス = BoneIndex
    };
} // namespace enishi::component
