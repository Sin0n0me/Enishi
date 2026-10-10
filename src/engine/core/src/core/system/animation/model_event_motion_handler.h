#pragma once
#include "animation_system.h"
#include "model_event_motion_config.h"
#include <core/system/collision/model_click_settings.h>
#include <string_view>
#include <sub_system/collision/interface_model_click_handler.h>
#include <sub_system/physics/interface_physics_world.h>

namespace enishi::core {
    enum class ModelEventError { PlaybackFailed, PhysicsFailed, InvalidAction };
    struct ModelMotionEvent {
        std::string_view kind;
        std::optional<types::BoneIndex> bone;
        std::optional<glm::vec3> position;
    };

    class ModelEventMotionHandler : public sub_system::IModelClickHandler {
      private:
        std::shared_ptr<ecs::Registry> registry;
        std::shared_ptr<AnimationSystem> animation;
        std::vector<ModelEventMotionRule> rules;
        std::shared_ptr<ModelClickSettings> click_settings;
        std::shared_ptr<ModelClickSettings> click_settings;
        std::shared_ptr<sub_system::IPhysicsWorld> physics;

      public:
        ModelEventMotionHandler(std::shared_ptr<ecs::Registry> registry,
            std::shared_ptr<AnimationSystem> animation,
            std::vector<ModelEventMotionRule> rules,
            std::shared_ptr<ModelClickSettings> click_settings,
            std::shared_ptr<ModelClickSettings> click_settings,
            std::shared_ptr<sub_system::IPhysicsWorld> physics = nullptr);
        void on_model_click(const types::ModelClick& event) override;
        // Other event producers (for example a random timer) use the same rule selection.
        [[nodiscard]] foundation::Result<bool, ModelEventError> trigger(
            types::HandleId entity, const ModelMotionEvent& event);

      private:
        foundation::Result<bool, ModelEventError> apply_impulse(types::HandleId entity,
            const ModelMotionEvent& event,
            const ModelEventMotionRule& rule);
        const ModelEventMotionRule* select_rule(
            const component::ModelComponent& model, const ModelMotionEvent& event) const;
    };
} // namespace enishi::core
