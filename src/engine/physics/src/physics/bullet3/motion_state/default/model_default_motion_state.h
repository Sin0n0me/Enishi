#pragma once
#include <LinearMath/btMotionState.h>
#include <btBulletCollisionCommon.h>
#include <btBulletDynamicsCommon.h>
#include <platform/physics/motion_state/interface_motion_state.h>

namespace enishi::physics::bullet3 {
    class ModelDefaultMotionState : public btDefaultMotionState, public platform::IMotionState {
      public:
        using btDefaultMotionState::btDefaultMotionState;

        void reset(platform::IPhysicsBoneView* const physics_bone) override;
        void set_offset(const glm::mat4& offset) override;
        void update_global_transform(platform::IPhysicsBoneView* const physics_bone) override;
        void reflect_global_transform(platform::IPhysicsBoneView* const physics_bone,
            platform::IBoneUpdater* const bone_updater) override;
    };
} // namespace enishi::physics::bullet3