#pragma once
#include <sub_system/ik/interface_ik_bone_view.h>
#include <vector>

namespace enishi::skinning_system {
    class IKBoneView final : public sub_system::IIKBoneView {
      private:
        glm::quat* const rotation;
        glm::mat4* const global;

      public:
        IKBoneView(glm::quat& rotation, glm::mat4& global) noexcept
            : rotation(&rotation)
            , global(&global) {
        }
        ~IKBoneView(void) noexcept override = default;

        IKBoneView(void) = delete;
        IKBoneView(const IKBoneView&) = delete;
        IKBoneView& operator=(const IKBoneView&) = delete;
        IKBoneView(IKBoneView&&) = delete;
        IKBoneView& operator=(IKBoneView&&) = delete;

        void set_ik_rotation(glm::quat&& rotation) noexcept override;

        void set_ik_rotation(const glm::quat& rotation) noexcept override;

        glm::quat& get_ik_rotation(void) noexcept override;

        const glm::quat& get_ik_rotation(void) const noexcept override;

        glm::mat4 get_ik_global_transform(void) const noexcept override;

        void set_ik_global_transform(const glm::mat4& mat) noexcept override;

        void set_ik_global_transform(glm::mat4&& mat) noexcept override;
    };
} // namespace enishi::skinning_system