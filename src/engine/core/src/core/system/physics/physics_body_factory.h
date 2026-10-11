#pragma once
#include <component/physics_bodies_component.h>
#include <memory>
#include <sub_system/physics/interface_physics_world.h>
#include <skinning_system/cache/physics_bone_cache.h>

namespace enishi::core {
    /**
     * モデルが持つ剛体・ジョイントの定義データ(component::PhysicsBodiesComponent)から、
     * 実際にsub_system::IPhysicsWorld側のオブジェクト(剛体・ジョイント)を生成する
     */
    class PhysicsBodyFactory final {
      public:
        PhysicsBodyFactory(void) = delete;

        // 1モデル分の剛体/ジョイントをまとめて生成, worldへ登録する
        // Stop on failure to preserve rigid-body indices used by joints and morphs.
        [[nodiscard]] static foundation::Result<std::vector<types::PhysicsHandle>,
            sub_system::PhysicsError>
        build(sub_system::IPhysicsWorld& world,
            const component::PhysicsBodiesComponent& bodies,
            const std::shared_ptr<skinning_system::PhysicsBonesCache>& physics_cache,
            std::shared_ptr<sub_system::IBoneUpdater> updater) noexcept;
    };
} // namespace enishi::core
