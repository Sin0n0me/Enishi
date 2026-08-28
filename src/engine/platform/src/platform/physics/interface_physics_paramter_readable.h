#pragma once
#include <cstdint>

namespace enishi::platform {
    class IPhysicsParameterReadable {
      public:
        virtual ~IPhysicsParameterReadable(void) noexcept = default;

        [[nodiscard]] virtual bool can_update(void) const noexcept = 0;

        [[nodiscard]] virtual float get_step_time(void) const noexcept = 0;

        [[nodiscard]] virtual std::uint32_t get_max_step_count(void) const noexcept = 0;
    };
} // namespace enishi::platform