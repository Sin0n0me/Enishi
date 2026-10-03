#pragma once
#include <cstdint>
#include <glm/glm.hpp>
#include <sub_system/physics/bone/interface_physics_bone_view.h>
#include <sub_system/skinning_system/interface_bone_updater.h>

namespace enishi::sub_system {
    class IMotionState {
      public:
        virtual ~IMotionState(void) noexcept = default;

        virtual void set_offset(const glm::mat4& offset) = 0;

        virtual void reset(sub_system::IPhysicsBoneView* const physics_bone) = 0;

        // ローカル -> 物理エンジン
        virtual void update_global_transform(sub_system::IPhysicsBoneView* const physics_bone) = 0;

        // 物理エンジン -> ローカル
        virtual void reflect_global_transform(
            IPhysicsBoneView* const physics_bone, IBoneUpdater* const bone_updater) = 0;
    };
} // namespace enishi::sub_system