#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

namespace enishi::sub_system {
    class IIKBoneView {
      public:
        virtual ~IIKBoneView(void) noexcept = default;

        virtual void set_ik_rotation(glm::quat&& rotation) noexcept = 0;

        virtual void set_ik_rotation(const glm::quat& rotation) noexcept = 0;

        [[nodiscard]] virtual glm::quat& get_ik_rotation(void) noexcept = 0;

        [[nodiscard]] virtual const glm::quat& get_ik_rotation(void) const noexcept = 0;

        [[nodiscard]] virtual glm::mat4 get_ik_global_transform(void) const noexcept = 0;

        virtual void set_ik_global_transform(const glm::mat4& mat) noexcept = 0;

        virtual void set_ik_global_transform(glm::mat4&& mat) noexcept = 0;
    };
} // namespace enishi::sub_system