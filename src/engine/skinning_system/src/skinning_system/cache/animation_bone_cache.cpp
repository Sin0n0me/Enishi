#include "animation_bone_cache.h"

namespace enishi::skinning_system {
    AnimationBonesCache::AnimationBonesCache(std::span<const types::BoneNode> bone_nodes,
        std::vector<std::unique_ptr<AnimationBoneView>>&& animation_views)
        : bone_nodes(bone_nodes)
        , animation_views(std::move(animation_views)) {
    }

    std::span<const types::BoneNode> AnimationBonesCache::get_bone_nodes(void) const noexcept {
        return this->bone_nodes;
    }

    std::size_t AnimationBonesCache::size(void) const noexcept {
        return this->animation_views.size();
    }

    sub_system::IAnimationBoneView* AnimationBonesCache::at(const types::BoneIndex index) noexcept {
        return this->animation_views[index].get();
    }
    const sub_system::IAnimationBoneView* AnimationBonesCache::at(
        const types::BoneIndex index) const noexcept {
        return &*this->animation_views[index].get();
    }
    foundation::Option<sub_system::IAnimationBoneView*> AnimationBonesCache::get(
        const types::BoneIndex index) noexcept {
        if (this->animation_views.size() <= index) {
            return {};
        }
        return this->animation_views[index].get();
    }
    foundation::Option<const sub_system::IAnimationBoneView*> AnimationBonesCache::get(
        const types::BoneIndex index) const noexcept {
        if (this->animation_views.size() <= index) {
            return {};
        }
        return this->animation_views[index].get();
    }
} // namespace enishi::skinning_system