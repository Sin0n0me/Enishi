#pragma once
#include "interface_mmd_motion_state.h"
#include <LinearMath/btMotionState.h>
#include <glm/glm.hpp>
#include <memory>

namespace enishi::physics::bullet3 {
    class MMDDynamicAndBoneMergeMotionState : public IMMDMotionState {
      private:
        glm::mat4 global;
        glm::mat4 offset;
        glm::mat4 inverse_offset;
        btTransform transform;
        bool override_with_physics;

      public:
        explicit MMDDynamicAndBoneMergeMotionState(
            const glm::mat4& offset, const bool override_with_physics);
        virtual ~MMDDynamicAndBoneMergeMotionState(void) noexcept = default;

        void getWorldTransform(btTransform& worldTrans) const override;
        void setWorldTransform(const btTransform& worldTrans) override;
        void set_offset(const glm::mat4& offset) override;
        void reset(platform::IPhysicsBoneView* const physics_bone) override;
        void update_global_transform(platform::IPhysicsBoneView* const physics_bone) override;
        void reflect_global_transform(platform::IPhysicsBoneView* const physics_bone,
            platform::IBoneUpdater* const bone_updater) override;
    };
} // namespace enishi::physics::bullet3