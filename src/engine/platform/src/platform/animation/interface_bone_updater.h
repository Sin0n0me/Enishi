#pragma once
#include <engine_types/assets/model/addons/bone.h>
#include <engine_types/handle/renderer/render_handle.h>
#include <foundation/option/option.h>
#include <platform/errors/renderer_errors.h>

namespace enishi::platform {
    class IBoneUpdater {
      public:
        virtual ~IBoneUpdater(void) noexcept = default;

        virtual void update_animation_global(void) noexcept = 0;

        virtual void update_animation_children_global(void) noexcept = 0;
    };
} // namespace enishi::platform