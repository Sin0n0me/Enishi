#pragma once
#include "interface_physics_world.h"
#include <engine_types/system/delta_time.h>
#include <memory>

namespace enishi::platform {
    class IPhysicsEngine {
      public:
        virtual ~IPhysicsEngine(void) noexcept = default;

        [[nodiscard]] virtual std::shared_ptr<IPhysicsWorld> get_shared_world(void) noexcept = 0;

        [[nodiscard]] virtual IPhysicsWorld* get_world(void) noexcept = 0;

        [[nodiscard]] virtual const IPhysicsWorld* get_world(void) const noexcept = 0;

        [[nodiscard]] virtual void update(const types::DeltaTime& delta_time) const noexcept = 0;
    };
} // namespace enishi::platform