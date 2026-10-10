#pragma once
#include "animation_system.h"
#include "model_event_motion_config.h"
#include <string_view>
#include <sub_system/collision/interface_model_click_handler.h>

namespace enishi::core {
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

      public:
        ModelEventMotionHandler(std::shared_ptr<ecs::Registry> registry,
            std::shared_ptr<AnimationSystem> animation,
            std::vector<ModelEventMotionRule> rules);
        void on_model_click(const types::ModelClick& event) override;
        // Other event producers (for example a random timer) use the same rule selection.
        [[nodiscard]] foundation::Result<bool, animation::AnimationError> trigger(
            types::HandleId entity, const ModelMotionEvent& event);

      private:
        const ModelEventMotionRule* select_rule(
            const component::ModelComponent& model, const ModelMotionEvent& event) const;
    };
} // namespace enishi::core
