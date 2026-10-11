#include "bone_name_map.h"

namespace enishi::assets_system {
    BoneNameMap BoneNameMapConstructor::build_name_index(void) {
        const auto size = this->bone_names.size();
        BoneNameMap map;
        map.bone_names = std::move(this->bone_names);

        for (std::size_t i = 0; i < size; ++i) {
            map.name_map[map.bone_names[i]] = i;
        }

        return map;
    }

    foundation::Option<types::BoneIndex> BoneNameMap::find_bone_index(
        const foundation::UTF8& name) const {
        const auto it = this->name_map.find(name);
        if (it == this->name_map.end()) {
            return {};
        }
        return it->second;
    }
} // namespace enishi::assets_system