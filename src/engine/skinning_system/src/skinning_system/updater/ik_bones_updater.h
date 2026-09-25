#pragma once
#include <skinning_system/cache/ik_bone_cache.h>
#include <span>
#include <sub_system/model/bone/interface_bind_bone_view_list.h>
#include <sub_system/skinning_system/interface_bone_updater.h>

namespace enishi::skinning_system {
    // IK rotations are accumulated local corrections to the current animation pose.
    // The optional animation-local span must cover every bone and outlive this updater.
    // Without it, the bind pose is used as the base transform.
    class IKBonesUpdater final : public sub_system::IBoneUpdater {
      private:
        IKBoneCache* const ik_view;
        const sub_system::IBindBoneViewList* const bind_view;
        std::span<const glm::mat4> animation_local;

      public:
        IKBonesUpdater(IKBoneCache& ik_view,
            const sub_system::IBindBoneViewList& bind_view,
            std::span<const glm::mat4> animation_local = {}) noexcept;

        [[nodiscard]] std::span<const types::BoneNode> bone_nodes(void) const noexcept;

        void update_local(const types::BoneIndex index) noexcept override;
        void update_global(const types::BoneIndex index) noexcept override;
        void update_children_global(const types::BoneIndex index) noexcept override;
        void update_global_form_roots(void) noexcept override;
    };
} // namespace enishi::skinning_system
