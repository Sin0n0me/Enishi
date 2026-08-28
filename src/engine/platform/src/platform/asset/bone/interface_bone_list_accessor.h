#pragma once
#include "interface_bone_accessor.h"
#include <engine_types/assets/model/addons/bone.h>
#include <engine_types/handle/renderer/render_handle.h>
#include <foundation/option/option.h>
#include <platform/errors/renderer_errors.h>

namespace enishi::platform {
    class IBoneListAccessor {
      public:
        virtual ~IBoneListAccessor(void) noexcept = default;

        virtual foundation::Option<IBoneAccessor*> get_bone_accessor(
            const types::BoneIndex& index) noexcept = 0;

        virtual foundation::Option<const IBoneAccessor*> get_bone_accessor(
            const types::BoneIndex& index) const noexcept = 0;
    };
} // namespace enishi::platform