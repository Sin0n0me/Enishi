#include "bind_bone_view.h"

namespace enishi::skinning_system {
    const glm::mat4& BindBoneView::get_bind_local(void) const noexcept {
        return *this->bind_local;
    }

    const glm::mat4& BindBoneView::get_bind_global(void) const noexcept {
        return *this->bind_global;
    }

    const glm::mat4& BindBoneView::get_bind_global_inverse(void) const noexcept {
        return *this->bind_global_inverse;
    }
} // namespace enishi::skeleton
