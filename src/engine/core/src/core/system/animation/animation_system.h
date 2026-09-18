#pragma once
#include <component/animation_component.h>
#include <component/model_component.h>
#include <animation/interface_animation_controller.h>
#include <core/system/interface_system.h>
#include <ecs/registry.h>
#include <engine_types/assets/model/addons/bone.h>
#include <memory>
#include <unordered_map>

namespace enishi::core {
    class AnimationSystem : public ISystem {
      private:
        std::shared_ptr<ecs::Registry> registry;
        std::unordered_map<types::HandleId, std::shared_ptr<animation::IAnimationController>>
            controllers;

      public:
        explicit AnimationSystem(const std::shared_ptr<ecs::Registry> registry);

        void set_controller(types::HandleId entity,
            std::shared_ptr<animation::IAnimationController> controller);
        void remove_controller(const types::HandleId& entity);
        [[nodiscard]] std::shared_ptr<animation::IAnimationController> get_controller(
            const types::HandleId& entity) const;

        bool should_close(void) override;
        void pre_update(void) override;
        void post_update(void) override;
        void update(const types::DeltaTime& delta_time) override;
        void render(void) const override;

      private:
        void apply_clip(component::AnimationComponent& animation,
            const animation::IAnimationController& controller) const;
    };
} // namespace enishi::core
