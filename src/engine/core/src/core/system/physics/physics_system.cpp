#include "physics_system.h"
#include <component/animation_component.h>
#include <component/model_component.h>
#include <foundation/log/logger.h>

namespace enishi::core {
    PhysicsSystem::PhysicsSystem(std::shared_ptr<ecs::Registry> registry,
        std::shared_ptr<sub_system::IPhysicsEngine> physics_engine)
        : registry(registry)
        , physics_engine(std::move(physics_engine)) {
    }

    bool PhysicsSystem::should_close(void) {
        return false;
    }

    void PhysicsSystem::pre_update(void) {
    }

    void PhysicsSystem::update(const types::DeltaTime& delta_time) {
        this->physics_engine->update(delta_time);
        this->physics_engine->get_world()->apply_physics();
    }

    void PhysicsSystem::post_update(void) {
    }

    void PhysicsSystem::render(void) const {
    }
} // namespace enishi::core
