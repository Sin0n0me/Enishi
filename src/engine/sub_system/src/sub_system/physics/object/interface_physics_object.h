#pragma once
#include <memory>

namespace enishi::sub_system {
    class IPhysicsObject {
      public:
        virtual ~PhysicsObject(void) noexcept = default;
    };
} // namespace enishi::sub_system