#pragma once
#include <animation/errors/errors.h>
#include <animation/interface_animation_controller.h>
#include <component/animation_component.h>
#include <component/model_component.h>
#include <core/system/interface_system.h>
#include <ecs/registry.h>
#include <engine_types/assets/model/addons/bone.h>
#include <filesystem>
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

        void set_controller(const types::HandleId entity,
            std::shared_ptr<animation::IAnimationController> controller);
        void remove_controller(const types::HandleId& entity);
        [[nodiscard]] foundation::Result<void, animation::AnimationError> play_vmd(
            types::HandleId entity, const std::filesystem::path& path, bool looping = true);
        [[nodiscard]] std::shared_ptr<animation::IAnimationController> get_controller(
            const types::HandleId& entity) const;

        bool should_close(void) override;
        void pre_update(void) override;
        void post_update(void) override;
        void update(const types::DeltaTime& delta_time) override;
        void render(void) const override;

    };
} // namespace enishi::core
