#pragma once
#include <cstdint>
#include <glm/glm.hpp>
#include <platform/animation/interface_bone_updater.h>
#include <platform/physics/bone/interface_physics_bone_view.h>

namespace enishi::platform {
    class IMotionState {
      public:
        virtual ~IMotionState(void) noexcept = default;

        virtual void set_offset(const glm::mat4& offset) = 0;

        virtual void reset(platform::IPhysicsBoneView* const physics_bone) = 0;

        // ローカル -> 物理エンジン
        virtual void update_global_transform(platform::IPhysicsBoneView* const physics_bone) = 0;

        // 物理エンジン -> ローカル
        virtual void reflect_global_transform(
            IPhysicsBoneView* const physics_bone, IBoneUpdater* const bone_updater) = 0;
    };
} // namespace enishi::platform