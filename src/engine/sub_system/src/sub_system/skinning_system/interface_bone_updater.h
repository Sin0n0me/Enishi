#pragma once
#include <engine_types/assets/model/addons/bone.h>
#include <engine_types/handle/renderer/render_handle.h>
#include <foundation/option/option.h>

namespace enishi::sub_system {
    class IBoneUpdater {
      public:
        virtual ~IBoneUpdater(void) noexcept = default;

        virtual void update_local(const types::BoneIndex index) noexcept = 0;

        virtual void update_global(const types::BoneIndex index) noexcept = 0;

        virtual void update_children_global(const types::BoneIndex index) noexcept = 0;

        virtual void update_global_form_roots(void) noexcept = 0;
    };
} // namespace enishi::sub_system