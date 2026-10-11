#pragma once
#include <sub_system/model/bone/interface_bind_bone_view.h>

namespace enishi::skinning_system {
    /**
     * sub_system::IBindBoneViewの非所有(non-owning)な実装
     * バインドポーズはモデル読み込み後は変化しないためすべて読み取り専用で保持する
     */
    class BindBoneView final : public sub_system::IBindBoneView {
      private:
        const glm::mat4* const bind_local;
        const glm::mat4* const bind_global;
        const glm::mat4* const bind_global_inverse;

      public:
        BindBoneView(const glm::mat4& bind_local,
            const glm::mat4& bind_global,
            const glm::mat4& bind_global_inverse) noexcept
            : bind_local(&bind_local)
            , bind_global(&bind_global)
            , bind_global_inverse(&bind_global_inverse) {
        }

        ~BindBoneView(void) noexcept override = default;

        BindBoneView(void) = delete;
        BindBoneView(const BindBoneView&) = delete;
        BindBoneView& operator=(const BindBoneView&) = delete;
        BindBoneView(BindBoneView&&) = delete;
        BindBoneView& operator=(BindBoneView&&) = delete;

        [[nodiscard]] const glm::mat4& get_bind_local(void) const noexcept override;
        [[nodiscard]] const glm::mat4& get_bind_global(void) const noexcept override;
        [[nodiscard]] const glm::mat4& get_bind_global_inverse(void) const noexcept override;
    };
} // namespace enishi::skinning_system
