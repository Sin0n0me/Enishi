#include "bind_bone_cache.h"

namespace enishi::skinning_system {
    BindBonesCache::BindBonesCache(std::vector<std::unique_ptr<BindBoneView>>&& bind_views)
        : bind_views(std::move(bind_views)) {
    }

    std::size_t BindBonesCache::size(void) const noexcept {
        return this->bind_views.size();
    }

    const sub_system::IBindBoneView* BindBonesCache::at(const types::BoneIndex index) const noexcept {
        return this->bind_views[index].get();
    }
} // namespace enishi::skinning_system