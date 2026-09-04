#include "physics_system.h"
#include <component/animation_component.h>
#include <component/model_component.h>
#include <foundation/log/logger.h>

namespace enishi::core {
    PhysicsSystem::PhysicsSystem(std::shared_ptr<ecs::Registory> registory,
        std::unique_ptr<platform::IPhysicsEngine> physics_engine)
        : registory(registory)
        , physics_engine(std::move(physics_engine)) {
    }

    bool PhysicsSystem::should_close(void) {
        return false;
    }

    void PhysicsSystem::pre_update(void) {
    }

    void PhysicsSystem::update(const types::DeltaTime& delta_time) {
        this->physics_engine->update(delta_time);
    }

    void PhysicsSystem::post_update(void) {
    }

    void PhysicsSystem::render(void) const {
    }
} // namespace enishi::core