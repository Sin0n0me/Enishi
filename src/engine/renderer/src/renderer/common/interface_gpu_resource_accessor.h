#pragma once
#include <foundation/option/option.h>
#include <foundation/result/result.h>
#include <memory>
#include <platform/renderer/interface_image_view.h>
#include <renderer/common/mesh/interface_mesh_accessor.h>
#include <renderer/common/shader/interface_shader_accessor.h>
#include <renderer/common/state/interface_state_accessor.h>
#include <renderer/common/view/interface_view_accessor.h>
#include <vector>

namespace enishi::renderer {
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
} // namespace enishi::renderer