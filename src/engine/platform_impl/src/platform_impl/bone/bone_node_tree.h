#pragma once
#include <engine_types/assets/model/addons/bone.h>
#include <memory>
#include <vector>

namespace enishi::platform_impl {
    class BoneNode;
    class BoneNodeTree {
      private:
        std::vector<std::shared_ptr<BoneNode>> bone_nodes;

      public:
        std::shared_ptr<BoneNode> get_node(const types::BoneIndex& index) const;
    };
} // namespace enishi::platform_impl