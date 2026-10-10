#include "model_event_motion_handler.h"
#include <component/physics_bodies_component.h>

namespace enishi::core {
    foundation::Result<bool, ModelEventError> ModelEventMotionHandler::apply_impulse(
        types::HandleId entity, const ModelMotionEvent& event, const ModelEventMotionRule& rule) {
        if (!event.bone.has_value() || !event.position.has_value()) {
            return false;
        }
        if (this->physics == nullptr) {
            return foundation::Error(ModelEventError::PhysicsFailed, "Physics world is missing");
        }
        const auto bodies = this->registry->get<component::PhysicsBodiesComponent>(entity);
        if (bodies.is_none() || !bodies.unwrap().enabled || !bodies.unwrap().simulation_enabled ||
            !this->physics->get_config_reader()->can_update()) {
            if (event.kind == "click") {
                this->click_settings->last_result = "Physics disabled or bodies unavailable";
            }
            return false;
        }
        const auto& state = bodies.unwrap();
        types::RigidBodyImpulse impulse;
        impulse.world_velocity = {rule.impulse[0], rule.impulse[1], rule.impulse[2]};
        impulse.world_position = event.position;
        bool applied = false;
        bool kinematic = false;
        bool zero_mass = false;
        for (std::size_t index = 0; index < state.rigid_bodies.size(); ++index) {
            const auto& body = state.rigid_bodies[index];
            if (body.relate_bone_index != *event.bone ||
                (event.kind == "click" && !this->click_settings->accepts_push(body.kind))) {
                continue;
            }
            if (body.kind == types::RigidBodyKind::Kinematic) {
                kinematic = true;
                continue;
            }
            if (!(body.mass > 0.0f)) {
                zero_mass = true;
                continue;
            }
            if (!(index < state.handles.size())) {
                return foundation::Error(
                    ModelEventError::PhysicsFailed, "Clicked rigid body has no runtime handle");
            }
            const auto result = this->physics->apply_impulse(state.handles[index], impulse);
            if (result.is_err()) {
                return result.propagation(ModelEventError::PhysicsFailed)
                    .add_message("Cannot apply impulse to clicked bone");
            }
            applied = true;
        }
        if (event.kind == "click") {
            this->click_settings->last_result = applied ? "Impulse submitted" : "No matching body";
            if (kinematic) {
                this->click_settings->last_result +=
                    "; Kinematic hit: animation-driven, no physical impulse";
            }
            if (zero_mass) {
                this->click_settings->last_result += "; zero-mass body cannot move from an impulse";
            }
        }
        return applied;
    }
} // namespace enishi::core
