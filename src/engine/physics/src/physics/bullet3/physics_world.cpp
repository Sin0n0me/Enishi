#include "physics_world.h"

namespace enishi::physics::bullet3 {
    void PhysicsWorld::simulation(const types::DeltaTime& dt) {
        if (!this->parameter->can_update()) {
            return;
        }

        const auto step_time = this->parameter->get_step_time();
        const auto max_step = this->parameter->get_max_step_count();

        this->world->stepSimulation(
            static_cast<btScalar>(dt.to_float()), max_step, static_cast<btScalar>(step_time));
    }
} // namespace enishi::physics::bullet3