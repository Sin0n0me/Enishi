#pragma once
#include "motion_state/interface_native_motion_satate_accessor.h"
#include "rigid_body/interface_native_rigid_body_accessor.h"
#include "shape/interface_native_shape_accessor.h"
#include <engine_types/handle/handle_type.h>
#include <foundation/option/option.h>
#include <memory>

namespace enishi::physics::bullet3 {
    class INativePhysicsAccessor {
      public:
        virtual ~INativePhysicsAccessor(void) noexcept = default;

        [[nodiscard]] virtual INativeRigidBodyAccessor* get_native_rigid_body_accessor(
            void) noexcept = 0;
        [[nodiscard]] virtual const INativeRigidBodyAccessor* get_native_rigid_body_accessor(
            void) const noexcept = 0;
        [[nodiscard]] virtual INativeMotionStateAccessor* get_native_motion_state_accessor(
            void) noexcept = 0;
        [[nodiscard]] virtual const INativeMotionStateAccessor* get_native_motion_state_accessor(
            void) const noexcept = 0;
        [[nodiscard]] virtual INativeShapeAccessor* get_native_shape_accessor(void) noexcept = 0;
        [[nodiscard]] virtual const INativeShapeAccessor* get_native_shape_accessor(
            void) const noexcept = 0;
    };
} // namespace enishi::physics::bullet3