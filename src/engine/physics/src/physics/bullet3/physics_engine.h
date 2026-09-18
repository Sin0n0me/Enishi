#pragma once
#include "physics_world.h"
#include <sub_system/physics/interface_physics_engine.h>

namespace enishi::physics::bullet3 {
    class PhysicsEngine : public sub_system::IPhysicsEngine {
      private:
        std::shared_ptr<PhysicsWorld> world;

      public:
        explicit PhysicsEngine(std::shared_ptr<sub_system::IPhysicsWorldConfigWriter> config);

        foundation::VoidResult<platform::PhysicsError> init_world(void) noexcept override;
        std::shared_ptr<sub_system::IPhysicsWorld> get_shared_world(void) noexcept override;
        sub_system::IPhysicsWorld* get_world(void) noexcept override;
        const sub_system::IPhysicsWorld* get_world(void) const noexcept override;
        void update(const types::DeltaTime& delta_time) const noexcept override;
    };
} // namespace enishi::physics::bullet3