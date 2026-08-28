#pragma once
#include <engine_types/assets/model/addons/bone.h>
#include <engine_types/handle/renderer/render_handle.h>
#include <foundation/option/option.h>
#include <platform/errors/renderer_errors.h>

namespace enishi::platform {
    class IBoneUpdater {
      public:
        virtual ~IBoneUpdater(void) noexcept = default;

        virtual void set_bone_local(glm::mat4&& matrix) noexcept = 0;

        virtual void set_bone_local(const glm::mat4& matrix) noexcept = 0;

        virtual void set_bone_global(glm::mat4&& matrix) noexcept = 0;

        virtual void set_bone_global(const glm::mat4& matrix) noexcept = 0;

        virtual glm::mat4& get_bone_local(void) noexcept = 0;

        virtual glm::mat4& get_bone_global(void) noexcept = 0;

        virtual const types::BindBone& get_bind_bone(void) const noexcept = 0;

        virtual void update_local(void) noexcept = 0;

        virtual void update_global(void) noexcept = 0;

        virtual void update_children_global(void) noexcept = 0;
    };
} // namespace enishi::platform