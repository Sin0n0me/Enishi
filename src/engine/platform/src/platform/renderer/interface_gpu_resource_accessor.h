#pragma once
#include <foundation/option/option.h>
#include <foundation/result/result.h>
#include <memory>
#include <platform/renderer/mesh/interface_mesh_accessor.h>
#include <platform/renderer/shader/interface_shader_accessor.h>
#include <platform/renderer/state/interface_state_accessor.h>
#include <platform/renderer/view/interface_image_view.h>
#include <platform/renderer/view/interface_view_accessor.h>
#include <vector>

namespace enishi::platform {
    class IGPUResourceAccessor {
      public:
        virtual ~IGPUResourceAccessor(void) noexcept = default;

        [[nodiscard]] virtual IViewAccessor* get_view_accessor(void) noexcept = 0;
        [[nodiscard]] virtual const IViewAccessor* get_view_accessor(void) const noexcept = 0;
        [[nodiscard]] virtual IShaderAccessor* get_shader_accessor(void) noexcept = 0;
        [[nodiscard]] virtual const IShaderAccessor* get_shader_accessor(void) const noexcept = 0;
        [[nodiscard]] virtual IMeshAccessor* get_mesh_accessor(void) noexcept = 0;
        [[nodiscard]] virtual const IMeshAccessor* get_mesh_accessor(void) const noexcept = 0;
        [[nodiscard]] virtual IStateAccessor* get_state_accessor(void) noexcept = 0;
        [[nodiscard]] virtual const IStateAccessor* get_state_accessor(void) const noexcept = 0;
    };
} // namespace enishi::platform