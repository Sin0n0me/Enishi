#include "model_event_motion_handler.h"
#include <foundation/log/logger.h>
#include <limits>

namespace enishi::core {
    ModelEventMotionHandler::ModelEventMotionHandler(std::shared_ptr<ecs::Registry> registry,
        std::shared_ptr<AnimationSystem> animation,
        std::vector<ModelEventMotionRule> rules,
        std::shared_ptr<sub_system::IPhysicsWorld> physics)
        : registry(std::move(registry))
        , animation(std::move(animation))
        , rules(std::move(rules))
        , physics(std::move(physics)) {
    }

    const ModelEventMotionRule* ModelEventMotionHandler::select_rule(
        const component::ModelComponent& model, const ModelMotionEvent& event) const {
        const ModelEventMotionRule* selected = nullptr;
        for (const auto& rule : this->rules) {
            if (!rule.enabled || rule.event != event.kind ||
                (rule.model != "*" && model.name.find(rule.model) == std::string::npos &&
                    model.source_path.find(rule.model) == std::string::npos)) {
                continue;
            }
            if (rule.bone != "*" &&
                (!event.bone.has_value() || !(*event.bone < model.bone_names.size()) ||
                    model.bone_names[*event.bone] != rule.bone)) {
                continue;
            }
            // Prefer bone-specific, then model-specific rules; file order breaks equal ties.
            if (selected == nullptr || (selected->bone == "*" && rule.bone != "*") ||
                ((selected->bone == "*") == (rule.bone == "*") && selected->model == "*" &&
                    rule.model != "*")) {
                selected = &rule;
            }
        }
        return selected;
    }

    foundation::Result<bool, ModelEventError> ModelEventMotionHandler::trigger(
        types::HandleId entity, const ModelMotionEvent& event) {
        const auto model = this->registry->get<component::ModelComponent>(entity);
        if (model.is_none() || !model.unwrap().visible) {
            return false;
        }
        const auto* rule = this->select_rule(model.unwrap(), event);
        if (rule == nullptr) {
            return false;
        }
        switch (rule->action) {
            case ModelEventAction::None:
                return false;
            case ModelEventAction::Impulse:
                return this->apply_impulse(entity, event, *rule);
            case ModelEventAction::Motion:
                break;
            default:
                return foundation::Error(
                    ModelEventError::InvalidAction, "Unknown model event action");
        }
        if (this->animation == nullptr) {
            return foundation::Error(
                ModelEventError::PlaybackFailed, "Animation system is missing");
        }
        const auto result = this->animation->play_vmd(entity, rule->motion, rule->looping);
        if (result.is_err()) {
            return result.propagation(ModelEventError::PlaybackFailed)
                .add_message("Cannot play configured model event motion");
        }
        return true;
    }

    void ModelEventMotionHandler::on_model_click(const types::ModelClick& event) {
        if (!event.hit.has_value() ||
            event.hit->model > std::numeric_limits<types::HandleId::HandleType>::max()) {
            return;
        }
        const types::HandleId entity(static_cast<types::HandleId::HandleType>(event.hit->model));
        const auto result =
            this->trigger(entity, ModelMotionEvent{"click", event.hit->bone, event.hit->position});
        if (result.is_err()) {
            foundation::Logger::warning(result.unwrap_err().get_message());
        }
    }
} // namespace enishi::core
