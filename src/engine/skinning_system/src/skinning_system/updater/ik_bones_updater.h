#pragma once
#include <sub_system/model/bone/interface_bind_bone_view_list.h>
#include <sub_system/skinning_system/interface_bone_updater.h>
#include <skinning_system/cache/ik_bone_cache.h>
#include <span>

namespace enishi::skinning_system {
    /**
     * この型の目的
     *
     * IKBoneCacheの階層を辿ってglobalを更新する
     *
     * IKComponentは各ボーンのローカル位置(pivot)を持たないため、
     * バインドポーズのローカル位置(不変)をpivotとして使う(IK対象ボーンは
     * 基本的にアニメーションで平行移動しない、という前提)
     * この前提が崩れる場合は設計の見直しが必要
     *
     * また、set_ik_rotationは「差分」ではなく「そのボーンの現在の絶対回転」として扱う
     * (CCDの反復のたびに毎回bind位置から計算し直すため、繰り返し呼んでも回転が積み重ならない)
     */
    class IKBonesUpdater final : public sub_system::IBoneUpdater {
      private:
        IKBoneCache* const ik_view;
        const sub_system::IBindBoneViewList* const bind_view;

      public:
        IKBonesUpdater(IKBoneCache& ik_view, const sub_system::IBindBoneViewList& bind_view) noexcept;

        [[nodiscard]] std::span<const types::BoneNode> bone_nodes(void) const noexcept;

        void update_local(const types::BoneIndex index) noexcept override;
        void update_global(const types::BoneIndex index) noexcept override;
        void update_children_global(const types::BoneIndex index) noexcept override;
        void update_global_form_roots(void) noexcept override;
    };
} // namespace enishi::skinning_system
