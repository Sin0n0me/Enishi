#include "ik_bone_view.h"

namespace enishi::skinning_system {
    void IKBoneView::set_ik_rotation(glm::quat&& rotation) noexcept {
        *this->rotation = std::move(rotation);
    }

    void IKBoneView::set_ik_rotation(const glm::quat& rotation) noexcept {
        *this->rotation = rotation;
    }

    glm::quat& IKBoneView::get_ik_rotation(void) noexcept {
        return *this->rotation;
    }

    const glm::quat& IKBoneView::get_ik_rotation(void) const noexcept {
        return *this->rotation;
    }

    glm::mat4 IKBoneView::get_ik_global_transform(void) const noexcept {
        return *this->global;
    }

    void IKBoneView::set_ik_global_transform(const glm::mat4& mat) noexcept {
        *this->global = mat;
    }

    void IKBoneView::set_ik_global_transform(glm::mat4&& mat) noexcept {
        *this->global = std::move(mat);
    }
} // namespace enishi::skinning_system
