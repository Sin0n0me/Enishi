#include "bone_resolver.h"

namespace enishi::assets_system {
    assets_system::BoneResolver::BoneResolver(BoneNameMapConstructor& bone_data)
        : name_map(bone_data.build_name_index()) {
    }

    foundation::Option<types::BoneIndex> BoneResolver::resolve_index(
        const foundation::UTF8& bone_name) const noexcept {
        const auto it = this->name_map.find_bone_index(bone_name);
        if (it.is_none()) {
            return {};
        }
        return it.unwrap();
    }

    foundation::Option<foundation::UTF8> BoneResolver::resolve_name(
        const types::BoneIndex bone_index) const noexcept {
        auto& bone_names = this->name_map.bone_names;
        if (bone_index >= bone_names.size()) {
            return {};
        }
        return bone_names[bone_index];
    }

    std::uint32_t BoneResolver::bone_count(void) const noexcept {
        return static_cast<std::uint32_t>(this->name_map.bone_names.size());
    }
} // namespace enishi::assets_system