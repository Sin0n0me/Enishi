#pragma once
#include <glm/glm.hpp>

namespace enishi::platform {
    class IBoneNodeView {
      public:
        virtual ~IBoneNodeView(void) noexcept = default;

        virtual const glm::mat4& get_bind_local(void) const noexcept = 0;

        virtual const glm::mat4& get_bind_global(void) const noexcept = 0;

        virtual const glm::mat4& get_bind_global_inverse(void) const noexcept = 0;

        virtual void write_local_from_bind(void) noexcept = 0;

        virtual void write_global_from_bind(void) noexcept = 0;
    };
} // namespace enishi::platform