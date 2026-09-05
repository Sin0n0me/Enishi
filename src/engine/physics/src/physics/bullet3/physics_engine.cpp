#include "physics_engine.h"

namespace enishi::physics::bullet3 {
    PhysicsEngine::PhysicsEngine(std::shared_ptr<platform::IPhysicsWorldConfigWriter> config)
        : world(std::make_shared<PhysicsWorld>(config)) {
    }

    foundation::VoidResult<platform::PhysicsError> PhysicsEngine::init_world(void) noexcept {
        auto&& result = this->world->init().add_message("物理世界の初期化に失敗しました");
        if (result.is_err()) {
            return result.propagation(platform::PhysicsError::InitError);
        }

        return {};
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