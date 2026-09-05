#pragma once
#include <memory>

namespace enishi::platform {
    class IPhysicsObject {
      public:
        virtual ~PhysicsObject(void) noexcept = default;
    };
} // namespace enishi::platform