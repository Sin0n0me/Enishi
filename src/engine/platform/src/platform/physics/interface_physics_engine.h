#pragma once

namespace enishi::platform {
    class IPhysicsEngine {
      public:
        virtual ~IPhysicsEngine(void) noexcept = default;
    };
} // namespace enishi::platform