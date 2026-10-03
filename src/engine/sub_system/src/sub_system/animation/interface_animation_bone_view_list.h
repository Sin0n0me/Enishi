#pragma once
#include "interface_animation_bone_view.h"
#include <cstddef>
#include <engine_types/assets/model/addons/bone.h>
#include <foundation/option/option.h>

namespace enishi::sub_system {
    /**
     * この型の目的
     *
     * モデル1体分のIAnimationBoneViewの集合へ、BoneIndexで任意アクセスするためのインターフェース
     * IKのチェーン計算のように、隣接ボーンに留まらず任意のボーンへアクセスしたい場合に使用する
     *
     * 実体(データの所有・ECSとの接続)がどこにあるかはこのインターフェースの関心事ではない
     */
    class IAnimationBoneViewList {
      public:
        virtual ~IAnimationBoneViewList(void) noexcept = default;

        [[nodiscard]] virtual std::size_t size(void) const noexcept = 0;

        [[nodiscard]] virtual IAnimationBoneView* at(const types::BoneIndex index) noexcept = 0;
        [[nodiscard]] virtual const IAnimationBoneView* at(
            const types::BoneIndex index) const noexcept = 0;

        [[nodiscard]] virtual foundation::Option<IAnimationBoneView*> get(
            const types::BoneIndex index) noexcept = 0;
        [[nodiscard]] virtual foundation::Option<const IAnimationBoneView*> get(
            const types::BoneIndex index) const noexcept = 0;
    };
} // namespace enishi::sub_system
