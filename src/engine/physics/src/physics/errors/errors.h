#pragma once
#include <cstdint>

namespace enishi::physics::bullet3 {
    enum class PhysicsError {
        ObjectError,
        RigidBodyError,
        MotionStateError,
    };
} // namespace enishi::physics::bullet3