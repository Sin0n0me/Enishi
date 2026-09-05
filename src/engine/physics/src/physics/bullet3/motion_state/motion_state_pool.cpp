#include "motion_state_pool.h"

namespace enishi::physics::bullet3 {
    std::tuple<types::HandleId, MotionStatePool::NativeMotionState&>
    MotionStatePool::emplace_native_motion_state(NativeMotionState&& motion_state) noexcept {
        return this->handle_mapper.make_from(
            this->native_motion_states.emplace(std::move(motion_state)),
            [](const std::size_t index) {
                return decltype(handle_mapper)::ValueType{
                    .index = index,
                };
            });
    }

    void MotionStatePool::remove_native_motion_state(const types::HandleId handle) noexcept {
    }

    foundation::Option<MotionStatePool::NativeMotionState&>
    MotionStatePool::get_native_motion_state(const types::HandleId handle) noexcept {
        return this->handle_mapper.get(handle).and_then(
            [this](const decltype(handle_mapper)::ValueType& mapped) {
                return this->native_motion_states.get(mapped.index);
            });
    }

    foundation::Option<const MotionStatePool::NativeMotionState&>
    MotionStatePool::get_native_motion_state(const types::HandleId handle) const noexcept {
        return this->handle_mapper.get(handle).and_then(
            [this](const decltype(handle_mapper)::ValueType& mapped) {
                return this->native_motion_states.get(mapped.index);
            });
    }

    foundation::Option<MotionStatePool::MotionState&> MotionStatePool::link_motion_state(
        const types::HandleId handle, MotionState&& motion_state) noexcept {
        if (this->handle_mapper.contains(handle)) {
            return {};
        }

        auto [index, ms] = this->motion_states.emplace(std::move(motion_state));
        this->handle_mapper[handle].config = index;

        return ms;
    }

    foundation::Option<MotionStatePool::MotionState&> MotionStatePool::get_motion_state(
        const types::HandleId handle) noexcept {
        return this->handle_mapper.get(handle).and_then(
            [this](const decltype(handle_mapper)::ValueType& mapped) {
                return this->motion_states.get(mapped.config);
            });
    }

    foundation::Option<const MotionStatePool::MotionState&> MotionStatePool::get_motion_state(
        const types::HandleId handle) const noexcept {
        return this->handle_mapper.get(handle).and_then(
            [this](const decltype(handle_mapper)::ValueType& mapped) {
                return this->motion_states.get(mapped.config);
            });
    }
} // namespace enishi::physics::bullet3