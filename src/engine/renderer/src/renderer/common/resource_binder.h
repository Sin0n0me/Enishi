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
        ResourcePool<ShaderBinding> shader_bindings;
        ResourcePool<ViewBinding> view_bindings;
        ResourcePool<StateBinding> state_bindings;

      public:
        std::tuple<types::HandleId, DrawBinding&> make_draw_binding(void) noexcept override;
        foundation::Option<const DrawBinding&> get_draw_binding(
            const types::HandleId handle) const noexcept override;
        foundation::Option<DrawBinding&> get_draw_binding(
            const types::HandleId handle) noexcept override;
        std::tuple<types::HandleId, BufferBinding&> make_buffer_binding(void) noexcept override;
        foundation::Option<const BufferBinding&> get_buffer_binding(
            const types::HandleId handle) const noexcept override;
        foundation::Option<BufferBinding&> get_buffer_binding(
            const types::HandleId handle) noexcept override;
        std::tuple<types::HandleId, TextureBinding&> make_texture_binding(void) noexcept override;
        foundation::Option<const TextureBinding&> get_texture_binding(
            const types::HandleId handle) const noexcept override;
        foundation::Option<TextureBinding&> get_texture_binding(
            const types::HandleId handle) noexcept override;
        std::tuple<types::HandleId, ShaderBinding&> make_shader_binding(void) noexcept override;
        foundation::Option<const ShaderBinding&> get_shader_binding(
            const types::HandleId handle) const noexcept override;
        foundation::Option<ShaderBinding&> get_shader_binding(
            const types::HandleId handle) noexcept override;
        std::tuple<types::HandleId, ViewBinding&> make_view_binding(void) noexcept override;
        foundation::Option<const ViewBinding&> get_view_binding(
            const types::HandleId handle) const noexcept override;
        foundation::Option<ViewBinding&> get_view_binding(
            const types::HandleId handle) noexcept override;
        std::tuple<types::HandleId, StateBinding&> make_state_binding(void) noexcept override;
        foundation::Option<const StateBinding&> get_state_binding(
            const types::HandleId handle) const noexcept override;
        foundation::Option<StateBinding&> get_state_binding(
            const types::HandleId handle) noexcept override;
    };
} // namespace enishi::renderer
