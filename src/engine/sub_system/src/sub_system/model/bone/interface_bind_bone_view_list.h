#pragma once
#include "interface_bind_bone_view.h"
#include <cstddef>
#include <engine_types/assets/model/addons/bone.h>

namespace enishi::sub_system {
    /**
     * この型の目的
     *
     * モデル1体分のIBindBoneViewの集合へ、BoneIndexで任意アクセスするためのインターフェース
     * バインドポーズは読み取り専用のため、constのatのみを持つ
     */
    class IBindBoneViewList {
      public:
        virtual ~IBindBoneViewList(void) noexcept = default;

        [[nodiscard]] virtual std::size_t size(void) const noexcept = 0;

        [[nodiscard]] virtual const IBindBoneView* at(
            const types::BoneIndex index) const noexcept = 0;
    };
} // namespace enishi::sub_system
