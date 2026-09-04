#pragma once
#include "interface_mmd_motion_state.h"
#include <btBulletCollisionCommon.h>
#include <btBulletDynamicsCommon.h>
#include <engine_types/handle/handle_type.h>
#include <foundation/option/option.h>
#include <memory>
#include <tuple>

namespace enishi::physics::bullet3 {
    class INativeMotionStateAccessor {
      public:
        using NativeMotionState = std::unique_ptr<IMMDMotionState>;

      public:
        virtual ~INativeMotionStateAccessor(void) noexcept = default;

        [[nodiscard]] virtual std::tuple<types::HandleId, NativeMotionState&>
        emplace_native_motion_state(NativeMotionState&& motion_state) noexcept = 0;
        [[nodiscard]] virtual std::tuple<types::HandleId, NativeMotionState&>
        make_native_motion_state(void) noexcept = 0;
        [[nodiscard]] virtual void remove_native_motion_state(
            const types::HandleId handle) noexcept = 0;

        [[nodiscard]] virtual foundation::Option<NativeMotionState&> get_native_motion_state(
            const types::HandleId handle) noexcept = 0;
        [[nodiscard]] virtual foundation::Option<const NativeMotionState&> get_native_motion_state(
            const types::HandleId handle) const noexcept = 0;
    };
} // namespace enishi::physics::bullet3