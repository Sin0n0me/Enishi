#pragma once
#include "../common/interface_shader_reflection.h"
#include <foundation/option/option.h>
#include <foundation/result/result.h>
#include <memory>
#include <platform/renderer/interface_image_view.h>
#include <renderer/common/interface_view_accessor.h>
#include <vector>

namespace enishi::renderer {
    class IGPUResourceAccessor {
      public:
        virtual ~IGPUResourceAccessor(void) noexcept = default;

        [[nodiscard]] virtual IViewAccessor* get_view_accessor(void) noexcept = 0;
        [[nodiscard]] virtual const IViewAccessor* get_view_accessor(void) const noexcept = 0;
    };
} // namespace enishi::renderer