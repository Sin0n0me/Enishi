#pragma once
#include <engine_types/handle/renderer/handles/resource_handles.h>
#include <engine_types/handle/renderer/render_handle.h>
#include <engine_types/renderer/binding_description.h>
#include <foundation/option/option.h>

namespace enishi::platform {
    class IRenderHandleMapper {
      public:
        virtual ~IRenderHandleMapper(void) noexcept = default;

        [[nodiscard]] virtual foundation::Option<const types::ResourceHandles&> get(
            const types::RenderHandle& handle) const noexcept = 0;
    };
} // namespace enishi::platform
