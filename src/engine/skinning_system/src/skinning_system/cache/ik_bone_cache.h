#pragma once
#include <foundation/option/option.h>
#include <memory>
#include <sub_system/ik/interface_ik_bone_view_list.h>
#include <skinning_system/views/ik_bone_view.h>
#include <span>
#include <vector>

namespace enishi::skinning_system {
    /**
     * モデル1体分のIKBoneViewをまとめて保持し、sub_system::IIKBoneViewListとして公開するキャッシュ
     * AnimationBonesCacheと同じ設計方針
     * resize時はこのキャッシュ自体を再作成する
     */
    class IKBoneCache final : public sub_system::IIKBoneViewList {
      private:
        std::span<const types::BoneNode> bone_nodes;
        std::vector<std::unique_ptr<IKBoneView>> ik_views;

      public:
        IKBoneCache(std::span<const types::BoneNode> bone_nodes,
            std::vector<std::unique_ptr<IKBoneView>>&& ik_views);

        [[nodiscard]] std::span<const types::BoneNode> get_bone_nodes(void) const noexcept;

        std::size_t size(void) const noexcept override;
        foundation::Option<sub_system::IIKBoneView*> get(
            const types::BoneIndex index) noexcept override;
        foundation::Option<const sub_system::IIKBoneView*> get(
            const types::BoneIndex index) const noexcept override;
        sub_system::IIKBoneView* at(const types::BoneIndex index) noexcept override;
        const sub_system::IIKBoneView* at(const types::BoneIndex index) const noexcept override;
    };
} // namespace enishi::skinning_system
