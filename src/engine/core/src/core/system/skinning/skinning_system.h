#pragma once
#include "model_bones.h"
#include <array>
#include <component/animation_component.h>
#include <component/ik_component.h>
#include <component/model_component.h>
#include <component/physics_bodies_component.h>
#include <component/physics_component.h>
#include <component/skinning_component.h>
#include <core/system/interface_system.h>
#include <ecs/registry.h>
#include <engine_types/skinning/skinning_command.h>
#include <memory>
#include <platform/physics/interface_physics_engine.h>
#include <span>
#include <unordered_map>
#include <variant>

namespace enishi::core {
    /**
     * モデルが持つ各ボーンデータを適切な順序で計算し
     * 最終的にスキニング用の行列を作成する
     */
    class SkinningSystem final : public ISystem {
      private:
        static constexpr std::array<types::SkinningCommand, 3> DEFAULT_ORDER = {
            types::SkinningCommand::Animation,
            types::SkinningCommand::IK,
            types::SkinningCommand::PhysicsSimulate,
        };

        using AddonComponent = std::variant<std::monostate,
            component::IKComponent*,
            component::PhysicsComponent*,
            component::PhysicsBodiesComponent*>;

      private:
        ecs::Registry* const registry;
        std::shared_ptr<platform::IPhysicsEngine> physics_engine;
        std::unordered_map<types::HandleId, std::unique_ptr<ModelBones>> model_bones;

      public:
        explicit SkinningSystem(ecs::Registry& registry) noexcept;

        bool should_close(void) override;
        void pre_update(void) override;
        void update(const types::DeltaTime& delta_time) override;
        void post_update(void) override;
        void render(void) const override;

      private:
        [[nodiscard]] ModelBones& get_or_build(const types::HandleId entity,
            const component::ModelComponent& model,
            component::AnimationComponent& animation,
            foundation::Option<component::IKComponent&> ik,
            foundation::Option<component::PhysicsComponent&> physics,
            foundation::Option<component::PhysicsBodiesComponent&> physics_bodies) noexcept;

        void solve_ik(
            ModelBones& bones, foundation::Option<component::IKComponent&> opt_ik) const noexcept;

        void execute_command(const types::SkinningCommand command,
            ModelBones& bones,
            foundation::Option<component::IKComponent&> ik) const noexcept;

        void write_skinning_matrices(const component::AnimationComponent& animation,
            const component::ModelComponent& model,
            component::SkinningComponent& skinning) const noexcept;
    };
} // namespace enishi::core
