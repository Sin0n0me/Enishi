#pragma once
#include <component/physics_bodies_component.h>
#include <engine_types/assets/model/model_data.h>
#include <foundation/option/option.h>

namespace enishi::core {
    /**
     * types::ModelData::addons(variantのリスト)から剛体・ジョイントの定義だけを取り出し、
     * component::PhysicsBodiesComponentへ変換する
     *
     * ここでは定義データのコピーのみを行い、Bullet3側のオブジェクト生成は行わない
     * (それはPhysicsBodyFactoryの責務)
     */
    class PhysicsBodiesComponentFactory final {
      public:
        PhysicsBodiesComponentFactory(void) = delete;

        // 剛体を1つも持たないモデルの場合はNoneを返す
        [[nodiscard]] static foundation::Option<component::PhysicsBodiesComponent> make(
            const types::ModelData& model_data) noexcept;
    };
} // namespace enishi::core
