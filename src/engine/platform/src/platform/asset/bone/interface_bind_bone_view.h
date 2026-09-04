#pragma once
#include <glm/glm.hpp>

namespace enishi::platform {
    class IBindBoneView {
      public:
        virtual ~IBindBoneView(void) noexcept = default;

        virtual const glm::mat4& get_bind_local(void) const noexcept = 0;

        virtual const glm::mat4& get_bind_global(void) const noexcept = 0;

        virtual const glm::mat4& get_bind_global_inverse(void) const noexcept = 0;
    };
} // namespace enishi::platform