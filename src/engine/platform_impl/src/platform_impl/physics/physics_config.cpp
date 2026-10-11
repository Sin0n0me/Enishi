#include "physics_config.h"

namespace enishi::platform_impl {
    PhysicsWorldConfig::PhysicsWorldConfig(void)
        : updatable(true)
        , fixed_step_time(1.0f / 60.0f)
        , max_step_count(2) {
    }

    bool PhysicsWorldConfig::can_update(void) const noexcept {
        return this->updatable;
    }
    float PhysicsWorldConfig::get_fixed_step_time(void) const noexcept {
        return this->fixed_step_time;
    }
    std::uint32_t PhysicsWorldConfig::get_max_step_count(void) const noexcept {
        return this->max_step_count;
    }

    void PhysicsWorldConfig::set_updatable(const bool can_update) noexcept {
        this->updatable = can_update;
    }
    void PhysicsWorldConfig::set_fixed_step_time(const float fixed_step_time) noexcept {
        this->fixed_step_time = fixed_step_time;
    }
    void PhysicsWorldConfig::set_max_step_count(const std::uint32_t max_step_count) noexcept {
        this->max_step_count = max_step_count;
    }
} // namespace enishi::platform_impl