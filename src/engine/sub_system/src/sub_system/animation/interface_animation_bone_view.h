#pragma once
#include <glm/glm.hpp>

namespace enishi::sub_system {
    class IAnimationBoneView {
      public:
        virtual ~IAnimationBoneView(void) noexcept = default;

        virtual void set_animation_translation(glm::vec3&& translation) noexcept = 0;

        virtual void set_animation_translation(const glm::mat4& translation) noexcept = 0;

        virtual void set_animation_rotation(glm::quat&& rotation) noexcept = 0;

        virtual void set_animation_rotation(const glm::quat& rotation) noexcept = 0;

        virtual void set_animation_scale(glm::vec3&& scale) noexcept = 0;

        virtual void set_animation_scale(const glm::vec3& scale) noexcept = 0;

        [[nodiscard]] virtual glm::vec3& get_animation_translation(void) noexcept = 0;

        [[nodiscard]] virtual const glm::vec3& get_animation_translation(void) const noexcept = 0;

        [[nodiscard]] virtual glm::quat& get_animation_rotation(void) noexcept = 0;

        [[nodiscard]] virtual const glm::quat& get_animation_rotation(void) const noexcept = 0;

        [[nodiscard]] virtual glm::vec3& get_animation_scale(void) noexcept = 0;

        [[nodiscard]] virtual const glm::vec3& get_animation_scale(void) const noexcept = 0;

        [[nodiscard]] virtual glm::mat4 get_animation_local_transform(void) const noexcept = 0;

        [[nodiscard]] virtual glm::mat4 get_animation_global_transform(void) const noexcept = 0;

        virtual void set_animation_global_transform(const glm::mat4& mat) noexcept = 0;

        virtual void set_animation_global_transform(glm::mat4&& mat) noexcept = 0;
    };
} // namespace enishi::sub_system