#pragma once
#include "interface_ik_bone_view.h"
#include <cstddef>
#include <engine_types/assets/model/addons/bone.h>
#include <foundation/option/option.h>

namespace enishi::sub_system {
    /**
     * IKチェーンを構成する複数ボーンへBoneIndexで任意アクセスするためのインターフェース
     */
    class IIKBoneViewList {
      public:
        virtual ~IIKBoneViewList(void) noexcept = default;

        [[nodiscard]] virtual std::size_t size(void) const noexcept = 0;

        [[nodiscard]] virtual foundation::Option<IIKBoneView*> get(
            const types::BoneIndex index) noexcept = 0;
        [[nodiscard]] virtual foundation::Option<const IIKBoneView*> get(
            const types::BoneIndex index) const noexcept = 0;

        [[nodiscard]] virtual IIKBoneView* at(const types::BoneIndex index) noexcept = 0;
        [[nodiscard]] virtual const IIKBoneView* at(
            const types::BoneIndex index) const noexcept = 0;
    };
} // namespace enishi::sub_system
