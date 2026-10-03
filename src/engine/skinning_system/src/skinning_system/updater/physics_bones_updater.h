#pragma once
#include <sub_system/skinning_system/interface_bone_updater.h>
#include <skinning_system/cache/physics_bone_cache.h>
#include <skinning_system/views/physics_bone_view.h>
#include <span>
#include <vector>

namespace enishi::skinning_system {
    class PhysicsBonesUpdater : public sub_system::IBoneUpdater {
      private:
        PhysicsBonesCache* const physics_view;

      public:
        explicit PhysicsBonesUpdater(PhysicsBonesCache& physics_view) noexcept
            : physics_view(&physics_view) {
        }

        [[nodiscard]] std::span<const types::BoneNode> bone_nodes(void) const noexcept {
            return this->physics_view->get_bone_nodes();
        }

        void update_local(const types::BoneIndex index) noexcept override;
        void update_global(const types::BoneIndex index) noexcept override;
        void update_children_global(const types::BoneIndex index) noexcept override;
        void update_global_form_roots(void) noexcept override;
    };
} // namespace enishi::skinning_system