#pragma once
#include "interface_mmd_motion_state.h"
#include <LinearMath/btMotionState.h>
#include <glm/glm.hpp>
#include <memory>
#include <platform/animation/updater/interface_bone_updater.h>

namespace enishi::physics::bullet3 {
    class MMDDynamicMotionState : public IMMDMotionState {
      private:
        std::shared_ptr<platform::IBoneUpdater> bone_node;
        glm::mat4 offset;
        glm::mat4 inverse_offset;
        btTransform transform;
        bool override_with_physics;

      public:
        explicit MMDDynamicMotionState(std::shared_ptr<platform::IBoneUpdater> bone_node,
            const glm::mat4& offset,
            const bool override_with_physics);
        virtual ~MMDDynamicMotionState(void) noexcept = default;

        void getWorldTransform(btTransform& worldTrans) const override;
        void setWorldTransform(const btTransform& worldTrans) override;
        void reset(void) override;
        void reflect_global_transform(void) override;
    };
} // namespace enishi::physics::bullet3