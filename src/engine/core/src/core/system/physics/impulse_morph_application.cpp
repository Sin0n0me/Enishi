#include "impulse_morph_application.h"
#include <core/system/animation/impulse_morph.h>

namespace enishi::core {
    foundation::Result<void, sub_system::PhysicsError> apply_impulse_morphs(
        sub_system::IPhysicsWorld& world,
        const component::ModelComponent& model,
        const component::MorphComponent& morph,
        const component::PhysicsBodiesComponent& bodies) {
        const auto impulses = evaluate_impulse_morphs(
            bodies.rigid_bodies.size(), model.morph_targets.targets, morph.weights);
        if (impulses.is_err()) {
            return impulses.propagation(sub_system::PhysicsError::MakeError);
        }
        for (std::size_t index = 0; index < impulses.unwrap().size(); ++index) {
            const auto& impulse = impulses.unwrap()[index];
            if (!impulse.reset_velocity && impulse.world_velocity == glm::vec3(0) &&
                impulse.world_torque == glm::vec3(0) && impulse.local_velocity == glm::vec3(0) &&
                impulse.local_torque == glm::vec3(0)) {
                continue;
            }
            if (bodies.handles.size() != bodies.rigid_bodies.size()) {
                return foundation::Error(sub_system::PhysicsError::MakeError,
                    "Impulse morph bodies have not been initialized");
            }
            auto result = world.apply_impulse(bodies.handles[index], impulse);
            if (result.is_err()) {
                return result;
            }
        }
        return {};
    }
} // namespace enishi::core
