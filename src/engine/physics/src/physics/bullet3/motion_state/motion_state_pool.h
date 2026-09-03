#pragma once
#include "interface_native_motion_satate_accessor.h"
#include <engine_types/handle/handle_mapper.h>
#include <foundation/pool/resource_pool.h>
#include <memory>
#include <vector>

namespace enishi::physics::bullet3 {
    class MotionStatePool : public INativeMotionStateAccessor {
      private:
        struct MotionStateHandle {
            std::size_t index;
        };

      private:
        types::ResourceMapper<MotionStateHandle> handle_mapper;
        foundation::ResourcePool<NativeMotionState> native_motion_states;

      public:
        std::tuple<types::HandleId, NativeMotionState&> emplace_native_motion_state(
            NativeMotionState&& motion_state) noexcept override;
        std::tuple<types::HandleId, NativeMotionState&> make_native_motion_state(
            void) noexcept override;
        void remove_native_motion_state(const types::HandleId handle) noexcept override;

        foundation::Option<NativeMotionState&> get_native_motion_state(
            const types::HandleId handle) noexcept override;
        foundation::Option<const NativeMotionState&> get_native_motion_state(
            const types::HandleId handle) const noexcept override;
    };
} // namespace enishi::physics::bullet3
