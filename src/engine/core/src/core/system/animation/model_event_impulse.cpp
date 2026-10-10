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
            return false;
        }
        const auto& state = bodies.unwrap();
        types::RigidBodyImpulse impulse;
        impulse.world_velocity = {rule.impulse[0], rule.impulse[1], rule.impulse[2]};
        impulse.world_position = event.position;
        bool applied = false;
        for (std::size_t index = 0; index < state.rigid_bodies.size(); ++index) {
            const auto& body = state.rigid_bodies[index];
            if (body.relate_bone_index != *event.bone ||
                (body.kind != types::RigidBodyKind::Dynamic &&
                    body.kind != types::RigidBodyKind::DynamicAdjustBone) ||
                !(body.mass > 0.0f)) {
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
        return applied;
    }
} // namespace enishi::core
