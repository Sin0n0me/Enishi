#pragma once
#include <cstdint>
#include <glm/glm.hpp>
#include <platform/animation/updater/interface_bone_updater.h>

namespace enishi::platform {
    class IMotionState {
      public:
        virtual ~IMotionState(void) noexcept = default;

        virtual void set_offset(const glm::mat4& offset) = 0;

        virtual void reset(IBoneUpdater* const bone_updater) = 0;

        // ローカル -> 物理エンジン
        virtual void update_global_transform(IBoneUpdater* const bone_updater) = 0;

        // 物理エンジン -> ローカル
        virtual void reflect_global_transform(IBoneUpdater* const bone_updater) = 0;
    };
} // namespace enishi::platform