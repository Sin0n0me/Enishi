#pragma once
#include <engine_types/assets/model/addons/bone.h>
#include <engine_types/handle/renderer/render_handle.h>
#include <foundation/option/option.h>
#include <platform/animation/updater/interface_bone_updater.h>
#include <platform/errors/renderer_errors.h>

namespace enishi::platform {
    class IBoneAccessor {
      public:
        virtual ~IBoneAccessor(void) noexcept = default;

        virtual glm::mat4& get_bone_local(void) noexcept = 0;

        virtual glm::mat4& get_bone_global(void) noexcept = 0;

        virtual const types::BindBone& get_bind_bone(void) const noexcept = 0;

        virtual IBoneUpdater* get_updater(void) noexcept = 0;

        virtual const IBoneUpdater* get_updater(void) const noexcept = 0;

        virtual foundation::Option<IBoneUpdater*> get_parent_updater(void) noexcept = 0;

        virtual foundation::Option<const IBoneUpdater*> get_parent_updater(void) const noexcept = 0;

        virtual foundation::Option<IBoneAccessor*> get_parent_accessor(void) noexcept = 0;

        virtual foundation::Option<const IBoneAccessor*> get_parent_accessor(
            void) const noexcept = 0;
    };
} // namespace enishi::platform