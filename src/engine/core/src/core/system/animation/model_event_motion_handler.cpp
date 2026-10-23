#include "model_event_motion_handler.h"
#include <component/physics_bodies_component.h>
#include <foundation/log/logger.h>
#include <limits>

namespace enishi::core {
    ModelEventMotionHandler::ModelEventMotionHandler(std::shared_ptr<ecs::Registry> registry,
        std::shared_ptr<AnimationSystem> animation,
        std::vector<ModelEventMotionRule> rules,
        std::shared_ptr<types::ModelClickSettings> click_settings,
        std::shared_ptr<sub_system::IPhysicsWorld> physics)
        : registry(std::move(registry))
        , animation(std::move(animation))
        , rules(std::move(rules))
        , click_settings(std::move(click_settings))
        , physics(std::move(physics)) {
    }

    const ModelEventMotionRule* ModelEventMotionHandler::select_rule(
        const component::ModelComponent& model, const ModelMotionEvent& event) const {
        const ModelEventMotionRule* selected = nullptr;
        for (const auto& rule : this->rules) {
            // Filter before choosing specificity so a motion cannot hide a push rule.
            if (event.kind == "click" && rule.action != ModelEventAction::None &&
                ((this->click_settings->mode == types::ModelClickMode::Touch &&
                     rule.action != ModelEventAction::Motion) ||
                    (this->click_settings->mode == types::ModelClickMode::Push &&
                        rule.action != ModelEventAction::Impulse))) {
                continue;
            }
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
        if (event.kind == "click" && (this->click_settings == nullptr ||
                                         this->click_settings->mode == types::ModelClickMode::None)) {
            return false;
        }
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
        if (this->click_settings == nullptr) {
            return;
        }
        ++this->click_settings->click_count;
        this->click_settings->last_target = "No eligible bone hit";
        this->click_settings->last_result = "No action";
        if (!event.hit.has_value() ||
            event.hit->model > (std::numeric_limits<types::HandleId::HandleType>::max)()) {
            return;
        }
        const types::HandleId entity(static_cast<types::HandleId::HandleType>(event.hit->model));
        const auto model = this->registry->get<component::ModelComponent>(entity);
        this->click_settings->last_target = "Bone " + std::to_string(event.hit->bone);
        if (model.is_some() && event.hit->bone < model.unwrap().bone_names.size()) {
            this->click_settings->last_target += ": " + model.unwrap().bone_names[event.hit->bone];
        }
        const auto bodies = this->registry->get<component::PhysicsBodiesComponent>(entity);
        if (bodies.is_some()) {
            for (const auto& body : bodies.unwrap().rigid_bodies) {
                if (body.relate_bone_index != event.hit->bone) {
                    continue;
                }
                switch (body.kind) {
                    case types::RigidBodyKind::Kinematic:
                        this->click_settings->last_target += " [Kinematic]";
                        break;
                    case types::RigidBodyKind::Dynamic:
                        this->click_settings->last_target += " [Dynamic]";
                        break;
                    case types::RigidBodyKind::DynamicAdjustBone:
                        this->click_settings->last_target += " [DynamicAdjustBone]";
                        break;
                }
            }
        }
        this->click_settings->last_result = "No enabled matching action (or mode None)";
        const auto result =
            this->trigger(entity, ModelMotionEvent{"click", event.hit->bone, event.hit->position});
        if (result.is_err()) {
            this->click_settings->last_result = result.unwrap_err().get_message();
            foundation::Logger::warning(result.unwrap_err().get_message());
        } else if (result.unwrap() && this->click_settings->mode == types::ModelClickMode::Touch) {
            this->click_settings->last_result = "Motion started";
        }
        foundation::Logger::info(
            this->click_settings->last_target + ": " + this->click_settings->last_result);
    }
} // namespace enishi::core
