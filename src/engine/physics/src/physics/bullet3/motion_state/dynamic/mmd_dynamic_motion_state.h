#pragma once
#include <LinearMath/btMotionState.h>
#include <glm/glm.hpp>
#include <memory>
#include <physics/bullet3/motion_state/interface_mmd_motion_state.h>

namespace enishi::physics::bullet3 {
    class MMDDynamicMotionState : public IMMDMotionState {
      private:
        glm::mat4 global;
        glm::mat4 offset;
        glm::mat4 inverse_offset;
        btTransform transform;
        bool override_with_physics;
        types::BoneIndex index;

      public:
        explicit MMDDynamicMotionState(const glm::mat4& offset, const bool override_with_physics);
        virtual ~MMDDynamicMotionState(void) noexcept = default;

        void getWorldTransform(btTransform& worldTrans) const override;
        void setWorldTransform(const btTransform& worldTrans) override;
        void set_offset(const glm::mat4& offset) override;
        void reset(sub_system::IPhysicsBoneView* const physics_bone) override;
        void update_global_transform(sub_system::IPhysicsBoneView* const physics_bone) override;
        void reflect_global_transform(sub_system::IPhysicsBoneView* const physics_bone,
            sub_system::IBoneUpdater* const bone_updater) override;
    };
} // namespace enishi::physics::bullet3