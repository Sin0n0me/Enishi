#pragma once
#include "handle_mapper.h"
#include "interface_resource_binder.h"
#include "resource_pool.h"

namespace enishi::renderer {
    class ResourceBinder : public IResourceBinder {
      private:
        struct BindHandle {
            std::size_t resource_index;
            std::size_t interface_index;
        };

      private:
        HandleMapper<BindHandle> handle_mapper;
        ResourcePool<BufferBinding> buffer_bindings;
        ResourcePool<DrawBinding> draw_bindings;
        ResourcePool<TextureBinding> texture_bindings;

      public:
        std::tuple<types::HandleId, DrawBinding&> make_draw_binding(void) noexcept override;
        foundation::Option<const DrawBinding&> get_draw_binding(
            const types::HandleId index) const noexcept override;
        std::tuple<types::HandleId, BufferBinding&> make_buffer_binding(void) noexcept override;
        foundation::Option<const BufferBinding&> get_buffer_binding(
            const types::HandleId index) const noexcept override;
        std::tuple<types::HandleId, TextureBinding&> make_texture_binding(void) noexcept override;
        foundation::Option<const TextureBinding&> get_texture_binding(
            const types::HandleId index) const noexcept override;
    };
} // namespace enishi::renderer
