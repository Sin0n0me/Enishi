#include "morph_name_map.h"

namespace enishi::assets_system {
    MorphNameMap MorphNameMapConstructor::build_name_index(void) {
        const auto size = this->morph_names.size();
        MorphNameMap map;
        map.morph_names = std::move(this->morph_names);
        for (std::size_t i = 0; i < size; ++i) {
            map.name_map[this->morph_names[i]] = i;
        }

        return map;
    }

    foundation::Option<types::MorphIndex> MorphNameMap::find_bone_index(
        const foundation::UTF8& name) const {
        const auto it = this->name_map.find(name);
        if (it == this->name_map.end()) {
            return {};
        }
        return it->second;
    }
} // namespace enishi::assets_system