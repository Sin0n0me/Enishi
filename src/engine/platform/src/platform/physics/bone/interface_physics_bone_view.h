#pragma once
#include <glm/glm.hpp>

namespace enishi::platform {
    class IPhysicsBoneView {
      public:
        virtual ~IPhysicsBoneView(void) noexcept = default;

        virtual void set_physics_local(glm::mat4&& local) noexcept = 0;

        virtual void set_physics_local(const glm::mat4& local) noexcept = 0;

        virtual void set_physics_global(glm::mat4&& global) noexcept = 0;

        virtual void set_physics_global(const glm::mat4& global) noexcept = 0;

        virtual glm::mat4& get_physics_local(void) noexcept = 0;

        virtual const glm::mat4& get_physics_local(void) const noexcept = 0;

        virtual glm::mat4& get_physics_global(void) noexcept = 0;

        virtual const glm::mat4& get_physics_global(void) const noexcept = 0;
    };
} // namespace enishi::platform