#pragma once
#include <engine_types/assets/model/bone.h>
#include <foundation/option/option.h>
#include <foundation/str/str.h>
#include <glm/glm.hpp>
#include <unordered_map>
#include <vector>

namespace enishi::assets_system {
    struct BoneNameMap {
        std::unordered_map<foundation::UTF8, types::BoneIndex> name_map;
        std::vector<foundation::UTF8> bone_names;

        // 名前からインデックスを引く
        [[nodiscard]]
        foundation::Option<types::BoneIndex> find_bone_index(const foundation::UTF8& name) const;
    };

    struct BoneNameMapConstructor {
        std::vector<foundation::UTF8> bone_names;

        // ロード時のみ呼ぶ
        BoneNameMap build_name_index(void);
    };
} // namespace enishi::assets_system
