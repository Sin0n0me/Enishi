#include "animation_bone_view.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

namespace enishi::skinning_system {
    void AnimationBoneView::set_animation_translation(glm::vec3&& translation) noexcept {
        *this->translation = std::move(translation);
    }

    void AnimationBoneView::set_animation_translation(const glm::mat4& translation) noexcept {
        *this->translation = glm::vec3(translation[3]);
    }

    void AnimationBoneView::set_animation_rotation(glm::quat&& rotation) noexcept {
        *this->rotation = std::move(rotation);
    }

    void AnimationBoneView::set_animation_rotation(const glm::quat& rotation) noexcept {
        *this->rotation = rotation;
    }

    void AnimationBoneView::set_animation_scale(glm::vec3&& scale) noexcept {
        *this->scale = std::move(scale);
    }

    void AnimationBoneView::set_animation_scale(const glm::vec3& scale) noexcept {
        *this->scale = scale;
    }

    glm::vec3& AnimationBoneView::get_animation_translation(void) noexcept {
        return *this->translation;
    }

    const glm::vec3& AnimationBoneView::get_animation_translation(void) const noexcept {
        return *this->translation;
    }

    glm::quat& AnimationBoneView::get_animation_rotation(void) noexcept {
        return *this->rotation;
    }

    const glm::quat& AnimationBoneView::get_animation_rotation(void) const noexcept {
        return *this->rotation;
    }

    glm::vec3& AnimationBoneView::get_animation_scale(void) noexcept {
        return *this->scale;
    }

    const glm::vec3& AnimationBoneView::get_animation_scale(void) const noexcept {
        return *this->scale;
    }

    glm::mat4 AnimationBoneView::get_animation_local_transform(void) const noexcept {
        const glm::mat4 IDENTITY{1.0f};
        const glm::mat4 t = glm::translate(IDENTITY, *this->translation);
        const glm::mat4 r = glm::mat4_cast(*this->rotation);
        const glm::mat4 s = glm::scale(IDENTITY, *this->scale);
        return t * r * s;
    }

    glm::mat4 AnimationBoneView::get_animation_global_transform(void) const noexcept {
        return *this->global;
    }

    void AnimationBoneView::set_animation_global_transform(const glm::mat4& mat) noexcept {
        *this->global = mat;
    }

    void AnimationBoneView::set_animation_global_transform(glm::mat4&& mat) noexcept {
        *this->global = std::move(mat);
    }
} // namespace enishi::skinning_system