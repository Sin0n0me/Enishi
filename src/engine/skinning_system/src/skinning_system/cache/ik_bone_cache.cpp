#include "ik_bone_cache.h"

namespace enishi::skinning_system {
    IKBoneCache::IKBoneCache(std::span<const types::BoneNode> bone_nodes,
        std::vector<std::unique_ptr<IKBoneView>>&& ik_views)
        : bone_nodes(bone_nodes)
        , ik_views(std::move(ik_views)) {
    }

    std::span<const types::BoneNode> IKBoneCache::get_bone_nodes(void) const noexcept {
        return this->bone_nodes;
    }

    std::size_t IKBoneCache::size(void) const noexcept {
        return this->ik_views.size();
    }

    foundation::Option<sub_system::IIKBoneView*> IKBoneCache::get(
        const types::BoneIndex index) noexcept {
        if (this->ik_views.size() <= index) {
            return {};
        }
        return this->ik_views[index].get();
    }
    foundation::Option<const sub_system::IIKBoneView*> IKBoneCache::get(
        const types::BoneIndex index) const noexcept {
        if (this->ik_views.size() <= index) {
            return {};
        }
        return this->ik_views[index].get();
    }
    sub_system::IIKBoneView* IKBoneCache::at(const types::BoneIndex index) noexcept {
        return this->ik_views[index].get();
    }
    const sub_system::IIKBoneView* IKBoneCache::at(const types::BoneIndex index) const noexcept {
        return this->ik_views[index].get();
    }
} // namespace enishi::skinning_system