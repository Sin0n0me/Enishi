#pragma once
#include "physics_world.h"
#include <platform/physics/interface_physics_engine.h>

namespace enishi::physics::bullet3 {
    class PhysicsEngine : public platform::IPhysicsEngine {
      private:
        std::shared_ptr<PhysicsWorld> world;

      public:
        explicit PhysicsEngine(void);

        std::shared_ptr<platform::IPhysicsWorld> get_shared_world(void) noexcept override;
        platform::IPhysicsWorld* get_world(void) noexcept override;
        const platform::IPhysicsWorld* get_world(void) const noexcept override;
        void update(const types::DeltaTime& delta_time) const noexcept override;
    };
} // namespace enishi::physics::bullet3