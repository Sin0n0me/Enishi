#include "physics_bone_view.h"

namespace enishi::skinning_system {
    void PhysicsBoneView::set_physics_local(glm::mat4&& local) noexcept {
        *this->local = std::move(local);
    }

    void PhysicsBoneView::set_physics_local(const glm::mat4& local) noexcept {
        *this->local = local;
    }

    void PhysicsBoneView::set_physics_global(glm::mat4&& global) noexcept {
        *this->global = std::move(global);
    }

    void PhysicsBoneView::set_physics_global(const glm::mat4& global) noexcept {
        *this->global = global;
    }

    glm::mat4& PhysicsBoneView::get_physics_local(void) noexcept {
        return *this->local;
    }

    const glm::mat4& PhysicsBoneView::get_physics_local(void) const noexcept {
        return *this->local;
    }

    glm::mat4& PhysicsBoneView::get_physics_global(void) noexcept {
        return *this->global;
    }

    const glm::mat4& PhysicsBoneView::get_physics_global(void) const noexcept {
        return *this->global;
    }
} // namespace enishi::skeleton
