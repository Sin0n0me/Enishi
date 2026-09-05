#pragma once
#include <LinearMath/btMotionState.h>
#include <platform/physics/motion_state/interface_motion_state.h>

namespace enishi::physics::bullet3 {
    class IMMDMotionState : public btMotionState, public platform::IMotionState {
      public:
        virtual ~IMMDMotionState(void) noexcept = default;
    };
} // namespace enishi::physics::bullet3