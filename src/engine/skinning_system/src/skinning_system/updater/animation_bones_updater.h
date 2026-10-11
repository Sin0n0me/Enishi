#pragma once
#include <sub_system/skinning_system/interface_bone_updater.h>
#include <skinning_system/cache/animation_bone_cache.h>
#include <skinning_system/views/animation_bone_view.h>
#include <span>
#include <vector>

namespace enishi::skinning_system {
    class AnimationBonesUpdater : public sub_system::IBoneUpdater {
      private:
        AnimationBonesCache* const animation_view;

      public:
        explicit AnimationBonesUpdater(AnimationBonesCache& animation_view) noexcept
            : animation_view(&animation_view) {
        }

        // rebuild()後のbone_nodesを都度取得する(保存すると古い状態を参照し続けてしまうため)
        [[nodiscard]] std::span<const types::BoneNode> bone_nodes(void) const noexcept {
            return this->animation_view->get_bone_nodes();
        }

        void update_local(const types::BoneIndex index) noexcept override;
        void update_global(const types::BoneIndex index) noexcept override;
        void update_children_global(const types::BoneIndex index) noexcept override;
        void update_global_form_roots(void) noexcept override;
    };
} // namespace enishi::skinning_system