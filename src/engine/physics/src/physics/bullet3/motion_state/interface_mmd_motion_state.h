#pragma once
#include <LinearMath/btMotionState.h>

namespace enishi::physics::bullet3 {
    class IMMDMotionState : public btMotionState {
      public:
        virtual ~IMMDMotionState(void) noexcept = default;

        virtual void reset(void) = 0;
        virtual void reflect_global_transform(void) = 0;
    };
} // namespace enishi::physics::bullet3