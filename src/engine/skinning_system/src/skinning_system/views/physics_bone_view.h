#pragma once
#include <sub_system/physics/bone/interface_physics_bone_view.h>

namespace enishi::skinning_system {
    /**
     * 物理演算専用のView
     */
    class PhysicsBoneView final : public sub_system::IPhysicsBoneView {
      private:
        glm::mat4* const local;
        glm::mat4* const global;

      public:
        PhysicsBoneView(glm::mat4& local, glm::mat4& global) noexcept
            : local(&local)
            , global(&global) {
        }

        ~PhysicsBoneView(void) noexcept override = default;

        PhysicsBoneView(void) = delete;
        PhysicsBoneView(const PhysicsBoneView&) = delete;
        PhysicsBoneView& operator=(const PhysicsBoneView&) = delete;
        PhysicsBoneView(PhysicsBoneView&&) = delete;
        PhysicsBoneView& operator=(PhysicsBoneView&&) = delete;

        void set_physics_local(glm::mat4&& local) noexcept override;
        void set_physics_local(const glm::mat4& local) noexcept override;
        void set_physics_global(glm::mat4&& global) noexcept override;
        void set_physics_global(const glm::mat4& global) noexcept override;

        [[nodiscard]] glm::mat4& get_physics_local(void) noexcept override;
        [[nodiscard]] const glm::mat4& get_physics_local(void) const noexcept override;
        [[nodiscard]] glm::mat4& get_physics_global(void) noexcept override;
        [[nodiscard]] const glm::mat4& get_physics_global(void) const noexcept override;
    };
} // namespace enishi::skinning_system
