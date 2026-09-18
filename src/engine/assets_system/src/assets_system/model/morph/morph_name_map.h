#pragma once
#include <engine_types/assets/model/addons/morph.h>
#include <foundation/option/option.h>
#include <foundation/str/str.h>
#include <glm/glm.hpp>
#include <unordered_map>
#include <vector>

namespace enishi::assets_system {
    struct MorphNameMap {
        std::unordered_map<foundation::UTF8, types::MorphIndex> name_map;
        std::vector<foundation::UTF8> morph_names;

        // 名前からインデックスを引く
        [[nodiscard]]
        foundation::Option<types::MorphIndex> find_bone_index(const foundation::UTF8& name) const;
    };

    struct MorphNameMapConstructor {
        std::vector<foundation::UTF8> morph_names;

        // ロード時のみ呼ぶ
        MorphNameMap build_name_index(void);
    };
} // namespace enishi::assets_system
