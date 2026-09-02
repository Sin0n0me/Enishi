#pragma once
#include "resource_pool.h"
#include <engine_types/handle/handle_allocator.h>
#include <engine_types/handle/renderer/handles/resource_handles.h>
#include <memory>
#include <platform/renderer/interface_render_handle_mapper.h>

namespace enishi::renderer {
    class RenderHandleMapper : public platform::IRenderHandleMapper {
      private:
        std::unique_ptr<types::HandleAllocator> handle_allocator;
        std::unordered_map<types::RenderHandle, types::ResourceHandles> handle_mapper;

      public:
        RenderHandleMapper(void);

        types::RenderHandle make(
            const types::RenderHandleType& handle_type, types::ResourceHandles&& mapped) noexcept;

        foundation::Option<types::ResourceHandles&> get(const types::RenderHandle& handle) noexcept;

        types::ResourceHandles& operator[](const types::RenderHandle& handle);

      public:
        foundation::Option<const types::ResourceHandles&> get(
            const types::RenderHandle& handle) const noexcept override;
    };
} // namespace enishi::renderer
