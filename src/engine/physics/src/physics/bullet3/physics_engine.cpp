#include "physics_engine.h"

namespace enishi::physics::bullet3 {
    PhysicsEngine::PhysicsEngine(void)
        : world(std::make_shared<PhysicsWorld>()) {
    }

    std::shared_ptr<platform::IPhysicsWorld> PhysicsEngine::get_shared_world(void) noexcept {
        return this->world;
    }

    platform::IPhysicsWorld* PhysicsEngine::get_world(void) noexcept {
        return this->world.get();
    }

    const platform::IPhysicsWorld* PhysicsEngine::get_world(void) const noexcept {
        return this->world.get();
    }

    void PhysicsEngine::update(const types::DeltaTime& delta_time) const noexcept {
        this->world->simulation(delta_time);
    }
} // namespace enishi::physics::bullet3