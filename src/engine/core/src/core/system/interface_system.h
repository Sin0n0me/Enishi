#pragma once

#include <engine_types/system/delta_time.h>

namespace enishi::core {
    class ISystem {
      public:
        virtual ~ISystem(void) noexcept = default;

        virtual bool should_close(void) = 0;
        virtual void pre_update(void) = 0;
        virtual void update(const types::DeltaTime& delta_time) = 0;
        virtual void post_update(void) = 0;
        virtual void render(void) const = 0;
    };
} // namespace enishi::core