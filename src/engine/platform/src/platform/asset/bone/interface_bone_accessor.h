#pragma once
#include <engine_types/assets/model/addons/bone.h>
#include <engine_types/handle/renderer/render_handle.h>
#include <foundation/option/option.h>
#include <memory>
#include <platform/animation/updater/interface_bone_updater.h>
#include <platform/errors/renderer_errors.h>

namespace enishi::platform {
    class IBoneAccessor {
      public:
        virtual ~IBoneAccessor(void) noexcept = default;

        virtual glm::mat4& get_bone_local(void) noexcept = 0;

        virtual glm::mat4& get_bone_global(void) noexcept = 0;

        virtual const types::BindBone& get_bind_bone(void) const noexcept = 0;

        virtual std::shared_ptr<IBoneUpdater> get_updater(void) noexcept = 0;

        virtual std::shared_ptr<const IBoneUpdater> get_updater(void) const noexcept = 0;

        virtual foundation::Option<std::shared_ptr<IBoneUpdater>> get_parent_updater(
            void) noexcept = 0;

        virtual foundation::Option<std::shared_ptr<const IBoneUpdater>> get_parent_updater(
            void) const noexcept = 0;

        virtual foundation::Option<std::shared_ptr<IBoneAccessor>> get_parent_accessor(
            void) noexcept = 0;

        virtual foundation::Option<std::shared_ptr<const IBoneAccessor>> get_parent_accessor(
            void) const noexcept = 0;
    };
} // namespace enishi::platform