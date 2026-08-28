#pragma once

namespace enishi::platform {
    class IPhysicsWorld {
      private:
      public:
        virtual ~IPhysicsWorld(void) = default;

        [[nodiscard]]
        virtual void simulation(const types::DeltaTime& dt) = 0;
    };
} // namespace enishi::platform