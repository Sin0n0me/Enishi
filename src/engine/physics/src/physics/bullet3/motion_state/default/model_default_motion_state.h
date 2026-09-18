#pragma once
#include <LinearMath/btMotionState.h>
#include <btBulletCollisionCommon.h>
#include <btBulletDynamicsCommon.h>
#include <sub_system/physics/motion_state/interface_motion_state.h>

namespace enishi::physics::bullet3 {
    class ModelDefaultMotionState : public btDefaultMotionState, public platform::IMotionState {
      public:
        using btDefaultMotionState::btDefaultMotionState;

        void reset(sub_system::IPhysicsBoneView* const physics_bone) override;
        void set_offset(const glm::mat4& offset) override;
        void update_global_transform(sub_system::IPhysicsBoneView* const physics_bone) override;
        void reflect_global_transform(sub_system::IPhysicsBoneView* const physics_bone,
            sub_system::IBoneUpdater* const bone_updater) override;
    };
} // namespace enishi::physics::bullet3