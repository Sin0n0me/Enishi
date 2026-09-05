#include "morph_resolver.h"

namespace enishi::assets_system {
    MorphResolver::MorphResolver(MorphNameMapConstructor& morph_data)
        : name_map(morph_data.build_name_index()) {
    }

    foundation::Option<types::MorphIndex> MorphResolver::resolve_index(
        const foundation::UTF8& morph_name) const noexcept {
        const auto it = this->name_map.find_bone_index(morph_name);
        if (it.is_none()) {
            return {};
        }
        return it.unwrap();
    }

    foundation::Option<foundation::UTF8> MorphResolver::resolve_name(
        const types::BoneIndex morph_index) const noexcept {
        auto& bone_names = this->name_map.morph_names;
        if (morph_index >= bone_names.size()) {
            return {};
        }
        return bone_names[morph_index];
    }

    std::uint32_t MorphResolver::index_count(void) const noexcept {
        return static_cast<std::uint32_t>(this->name_map.morph_names.size());
    }
} // namespace enishi::assets_system