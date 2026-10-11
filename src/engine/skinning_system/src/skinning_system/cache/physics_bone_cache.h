#pragma once
#include <foundation/option/option.h>
#include <memory>
#include <sub_system/physics/bone/interface_physics_bone_view_list.h>
#include <skinning_system/views/physics_bone_view.h>
#include <span>
#include <vector>

namespace enishi::skinning_system {
    /**
     * モデル1体分のPhysicsBoneViewをまとめて保持し、sub_system::IPhysicsBoneViewListとして
     * 公開するキャッシュ(AnimationBonesCacheと同じ設計方針)
     * resize時はこのキャッシュ自体を再作成する
     */
    class PhysicsBonesCache final : public sub_system::IPhysicsBoneViewList {
      private:
        std::span<const types::BoneNode> bone_nodes;
        std::vector<std::shared_ptr<PhysicsBoneView>> physics_views;

      public:
        explicit PhysicsBonesCache(std::span<const types::BoneNode> bone_nodes,
            std::vector<std::shared_ptr<PhysicsBoneView>>&& physics_views);

        [[nodiscard]] std::span<const types::BoneNode> get_bone_nodes(void) const noexcept;

        [[nodiscard]] foundation::Option<std::shared_ptr<sub_system::IPhysicsBoneView>> get_shared(
            const types::BoneIndex index) const noexcept;
        std::size_t size(void) const noexcept override;
        foundation::Option<sub_system::IPhysicsBoneView*> get(
            const types::BoneIndex index) noexcept override;
        foundation::Option<const sub_system::IPhysicsBoneView*> get(
            const types::BoneIndex index) const noexcept override;
        sub_system::IPhysicsBoneView* at(const types::BoneIndex index) noexcept override;
        const sub_system::IPhysicsBoneView* at(const types::BoneIndex index) const noexcept override;
    };
} // namespace enishi::skinning_system
