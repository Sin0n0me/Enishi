#pragma once
#include <sub_system/animation/interface_animation_bone_view.h>

namespace enishi::skinning_system {
    /**
     * ビューの生存期間中にアドレスが変わることがない
     * 変わる場合は新しく作り直される
     */
    class AnimationBoneView final : public sub_system::IAnimationBoneView {
      private:
        glm::vec3* const translation;
        glm::quat* const rotation;
        glm::vec3* const scale;
        glm::mat4* const global; //  計算済みのグローバル行列

      public:
        AnimationBoneView(glm::vec3& translation,
            glm::quat& rotation,
            glm::vec3& scale,
            glm::mat4& global) noexcept
            : translation(&translation)
            , rotation(&rotation)
            , scale(&scale)
            , global(&global) {
        }

        ~AnimationBoneView(void) noexcept override = default;

        AnimationBoneView(void) = delete;
        AnimationBoneView(const AnimationBoneView&) = delete;
        AnimationBoneView& operator=(const AnimationBoneView&) = delete;
        AnimationBoneView(AnimationBoneView&&) = delete;
        AnimationBoneView& operator=(AnimationBoneView&&) = delete;

        void set_animation_translation(glm::vec3&& translation) noexcept override;
        void set_animation_translation(const glm::mat4& translation) noexcept override;
        void set_animation_rotation(glm::quat&& rotation) noexcept override;
        void set_animation_rotation(const glm::quat& rotation) noexcept override;
        void set_animation_scale(glm::vec3&& scale) noexcept override;
        void set_animation_scale(const glm::vec3& scale) noexcept override;

        glm::vec3& get_animation_translation(void) noexcept override;
        const glm::vec3& get_animation_translation(void) const noexcept override;
        glm::quat& get_animation_rotation(void) noexcept override;
        const glm::quat& get_animation_rotation(void) const noexcept override;
        glm::vec3& get_animation_scale(void) noexcept override;
        const glm::vec3& get_animation_scale(void) const noexcept override;

        glm::mat4 get_animation_local_transform(void) const noexcept override;
        glm::mat4 get_animation_global_transform(void) const noexcept override;
        void set_animation_global_transform(const glm::mat4& mat) noexcept override;
        void set_animation_global_transform(glm::mat4&& mat) noexcept override;
    };
} // namespace enishi::skinning_system
