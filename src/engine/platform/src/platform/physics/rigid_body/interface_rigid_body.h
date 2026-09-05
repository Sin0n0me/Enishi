#pragma once
#include <cstdint>
#include <glm/glm.hpp>

namespace enishi::platform {
    class IRigidBody {
      public:
        virtual ~IRigidBody(void) noexcept = default;

        virtual void set_active(const bool active_flag) = 0;

        virtual void reset(void) = 0;

        virtual void reset_transform(void) = 0;

        virtual void apply_local_transform(void) = 0;

        virtual void apply_global_transform(void) = 0;
    };
} // namespace enishi::platform