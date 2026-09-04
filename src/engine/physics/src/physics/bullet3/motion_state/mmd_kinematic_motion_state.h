#pragma once
#include "interface_mmd_motion_state.h"
#include <LinearMath/btMotionState.h>
#include <glm/glm.hpp>
#include <memory>

namespace enishi::physics::bullet3 {
    class MMDKinematicMotionState : public IMMDMotionState {
      private:
        glm::mat4 global;
        glm::mat4 offset;
        btTransform transform;

      public:
        explicit MMDKinematicMotionState(const glm::mat4& offset) noexcept;
        virtual ~MMDKinematicMotionState(void) noexcept = default;

        void getWorldTransform(btTransform& worldTrans) const override;
        void setWorldTransform(const btTransform& worldTrans) override;
        void reset(platform::IPhysicsBoneView* const physics_bone) override;
        void set_offset(const glm::mat4& offset) override;
        void update_global_transform(platform::IPhysicsBoneView* const physics_bone) override;
        void reflect_global_transform(platform::IPhysicsBoneView* const physics_bone,
            platform::IBoneUpdater* const bone_updater) override;
    };
} // namespace enishi::physics::bullet3