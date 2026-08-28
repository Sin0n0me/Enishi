#pragma once
#include "interface_mmd_motion_state.h"
#include <LinearMath/btMotionState.h>
#include <glm/glm.hpp>
#include <memory>
#include <platform/animation/updater/interface_bone_updater.h>

namespace enishi::physics::bullet3 {
    class MMDKinematicMotionState : public IMMDMotionState {
      private:
        std::shared_ptr<platform::IBoneUpdater> bone_node;
        glm::mat4 offset;
        btTransform transform;

      public:
        explicit MMDKinematicMotionState(
            std::shared_ptr<platform::IBoneUpdater> bone_node, const glm::mat4& offset);
        virtual ~MMDKinematicMotionState(void) noexcept = default;

        void getWorldTransform(btTransform& worldTrans) const override;
        void setWorldTransform(const btTransform& worldTrans) override;
        void reset(void) override;
        void reflect_global_transform(void) override;
    };
} // namespace enishi::physics::bullet3