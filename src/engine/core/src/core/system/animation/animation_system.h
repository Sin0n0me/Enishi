#pragma once
#include <component/animation_component.h>
#include <component/ik_component.h>
#include <component/model_component.h>
#include <component/transform_component.h>
#include <core/system/interface_system.h>
#include <ecs/registry.h>
#include <engine_types/assets/model/addons/bone.h>
#include <memory>

namespace enishi::core {
    class AnimationSystem : public ISystem {
      private:
        std::shared_ptr<ecs::Registry> registry;

      public:
        explicit AnimationSystem(const std::shared_ptr<ecs::Registry> registry);

        bool should_close(void) override;
        void pre_update(void) override;
        void post_update(void) override;
        void update(const types::DeltaTime& delta_time) override;
        void render(void) const override;

      private:
        void animation(component::AnimationComponent& animation,
            const component::ModelComponent& model,
            const component::IKComponent& ik);
    };
} // namespace enishi::core