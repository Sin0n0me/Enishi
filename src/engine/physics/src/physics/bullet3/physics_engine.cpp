#include "physics_engine.h"

namespace enishi::physics::bullet3 {
    PhysicsEngine::PhysicsEngine(std::shared_ptr<sub_system::IPhysicsWorldConfigWriter> config)
        : world(std::make_shared<PhysicsWorld>(config)) {
    }

    foundation::VoidResult<sub_system::PhysicsError> PhysicsEngine::init_world(void) noexcept {
        auto&& result = this->world->init().add_message("物理世界の初期化に失敗しました");
        if (result.is_err()) {
            return result.propagation(sub_system::PhysicsError::InitError);
        }

        return {};
    }

    std::shared_ptr<sub_system::IPhysicsWorld> PhysicsEngine::get_shared_world(void) noexcept {
        return this->world;
    }

    sub_system::IPhysicsWorld* PhysicsEngine::get_world(void) noexcept {
        return this->world.get();
    }

    const sub_system::IPhysicsWorld* PhysicsEngine::get_world(void) const noexcept {
        return this->world.get();
    }

    void PhysicsEngine::update(const types::DeltaTime& delta_time) const noexcept {
        this->world->simulation(delta_time);
    }
} // namespace enishi::physics::bullet3
