#pragma once
#include "interface_resource_binder.h"
#include "resource_pool.h"

namespace enishi::renderer {
    class ResourceBinder : public IResourceBinder {
      private:
        ResourcePool<BufferBinding> buffer_bindings;
        ResourcePool<DrawBinding> draw_bindings;
        ResourcePool<TextureBinding> texture_bindings;

      public:
        std::tuple<std::size_t, DrawBinding&> make_draw_binding(void) noexcept override;
        foundation::Option<const DrawBinding&> get_draw_binding(
            const std::size_t index) const noexcept override;
        std::tuple<std::size_t, BufferBinding&> make_buffer_binding(void) noexcept override;
        foundation::Option<const BufferBinding&> get_buffer_binding(
            const std::size_t index) const noexcept override;
        std::tuple<std::size_t, TextureBinding&> make_texture_binding(void) noexcept override;
        foundation::Option<const TextureBinding&> get_texture_binding(
            const std::size_t index) const noexcept override;
    };
} // namespace enishi::renderer
