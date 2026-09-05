#pragma once
#include "interface_physics_world_config_reader.h"
#include <cstdint>

namespace enishi::platform {
    class IPhysicsWorldConfigWriter : public IPhysicsWorldConfigReader {
      public:
        virtual ~IPhysicsWorldConfigWriter(void) noexcept = default;

        virtual void set_updatable(const bool can_update) noexcept = 0;

        virtual void set_fixed_step_time(const float fixed_step_time) noexcept = 0;

        virtual void set_max_step_count(const std::uint32_t max_step_count) noexcept = 0;
    };
} // namespace enishi::platform