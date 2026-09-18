#pragma once
#include "interface_physics_bone_view.h"
#include <cstddef>
#include <engine_types/assets/model/addons/bone.h>
#include <foundation/option/option.h>

namespace enishi::sub_system {
    /**
     * モデル1体分のIPhysicsBoneViewの集合へBoneIndexで任意アクセスするためのインターフェース
     */
    class IPhysicsBoneViewList {
      public:
        virtual ~IPhysicsBoneViewList(void) noexcept = default;

        [[nodiscard]] virtual std::size_t size(void) const noexcept = 0;

        [[nodiscard]] virtual foundation::Option<IPhysicsBoneView*> get(
            const types::BoneIndex index) noexcept = 0;
        [[nodiscard]] virtual foundation::Option<const IPhysicsBoneView*> get(
            const types::BoneIndex index) const noexcept = 0;

        [[nodiscard]] virtual IPhysicsBoneView* at(const types::BoneIndex index) noexcept = 0;
        [[nodiscard]] virtual const IPhysicsBoneView* at(
            const types::BoneIndex index) const noexcept = 0;
    };
} // namespace enishi::sub_system
