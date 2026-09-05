#pragma once
#include <engine_types/assets/model/addons/bone.h>
#include <foundation/option/option.h>
#include <memory>
#include <platform/physics/interface_physics_world_config_writer.h>

namespace enishi::platform_impl {
    class PhysicsWorldConfig : public platform::IPhysicsWorldConfigWriter {
      private:
        bool updatable;
        float fixed_step_time;
        std::uint32_t max_step_count;

      public:
        explicit PhysicsWorldConfig(void);

        bool can_update(void) const noexcept override;
        float get_fixed_step_time(void) const noexcept override;
        std::uint32_t get_max_step_count(void) const noexcept override;

        void set_updatable(const bool can_update) noexcept override;
        void set_fixed_step_time(const float fixed_step_time) noexcept override;
        void set_max_step_count(const std::uint32_t max_step_count) noexcept override;
    };
} // namespace enishi::platform_impl