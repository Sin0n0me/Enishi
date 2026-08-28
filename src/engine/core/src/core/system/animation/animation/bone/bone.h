#pragma once
#include <engine_types/assets/model/addons/bone.h>
#include <platform/animation/updater/interface_bone_updater.h>
#include <platform/asset/bone/interface_bone_accessor.h>
#include <platform/asset/bone/interface_bone_list_accessor.h>
#include <platform/renderer/updater/interface_uniform_updater.h>

namespace enishi::core {
    // ボーンデータのやり取りはここで行う
    class Bone : public platform::IBoneAccessor,
                 public platform::IBoneUpdater
    //             public platform::IUnifromUpdater
    {
      private:
        std::weak_ptr<platform::IBoneListAccessor> bones_list;
        types::Bone bone;

      public:
        glm::mat4& get_bone_local(void) noexcept override;
        glm::mat4& get_bone_global(void) noexcept override;
        const types::BindBone& get_bind_bone(void) const noexcept override;
        IBoneUpdater* get_updater(void) noexcept override;
        const IBoneUpdater* get_updater(void) const noexcept override;
        foundation::Option<IBoneUpdater*> get_parent_updater(void) noexcept override;
        foundation::Option<const IBoneUpdater*> get_parent_updater(void) const noexcept override;
        foundation::Option<IBoneAccessor*> get_parent_accessor(void) noexcept override;
        foundation::Option<const IBoneAccessor*> get_parent_accessor(void) const noexcept override;

      public:
        void set_bone_local(glm::mat4&& matrix) noexcept override;
        void set_bone_local(const glm::mat4& matrix) noexcept override;
        void set_bone_global(glm::mat4&& matrix) noexcept override;
        void set_bone_global(const glm::mat4& matrix) noexcept override;
        void update_local(void) noexcept override;
        void update_global(void) noexcept override;
        void update_children_global(void) noexcept override;

      public:
        // void on_update(void) override;
        //  types::OwnedRenderData& get_resource(void) override;
    };
} // namespace enishi::core