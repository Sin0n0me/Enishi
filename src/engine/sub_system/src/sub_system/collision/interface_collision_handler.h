#pragma once
#include <engine_types/collider/collision.h>

namespace enishi::sub_system {
    class ICollisionHandler {
      public:
        virtual ~ICollisionHandler(void) noexcept = default;

        virtual void on_collision(
            types::BoneIndex bone_index, const types::Collision& collision) = 0;
    };
} // namespace enishi::sub_system
