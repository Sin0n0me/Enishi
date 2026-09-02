#pragma once
#include "resource_pool.h"
#include <engine_types/handle/handle_mapper.h>
#include <platform/renderer/interface_resource_binder.h>

namespace enishi::renderer {
    class ResourceBinder : public platform::IResourceBinder {
      private:
        struct BindHandle {
            std::size_t resource_index;
            std::size_t interface_index;
        };

      private:
        types::ResourceMapper<BindHandle> handle_mapper;
        ResourcePool<types::BufferBinding> buffer_bindings;
        ResourcePool<types::DrawBinding> draw_bindings;
        ResourcePool<types::TextureBinding> texture_bindings;
        ResourcePool<types::ShaderBinding> shader_bindings;
        ResourcePool<types::ViewBinding> view_bindings;
        ResourcePool<types::StateBinding> state_bindings;

      public:
        std::tuple<types::HandleId, types::DrawBinding&> make_draw_binding(void) noexcept override;
        foundation::Option<const types::DrawBinding&> get_draw_binding(
            const types::HandleId handle) const noexcept override;
        foundation::Option<types::DrawBinding&> get_draw_binding(
            const types::HandleId handle) noexcept override;
        std::tuple<types::HandleId, types::BufferBinding&> make_buffer_binding(
            void) noexcept override;
        foundation::Option<const types::BufferBinding&> get_buffer_binding(
            const types::HandleId handle) const noexcept override;
        foundation::Option<types::BufferBinding&> get_buffer_binding(
            const types::HandleId handle) noexcept override;
        std::tuple<types::HandleId, types::TextureBinding&> make_image_binding(
            void) noexcept override;
        foundation::Option<const types::TextureBinding&> get_texture_binding(
            const types::HandleId handle) const noexcept override;
        foundation::Option<types::TextureBinding&> get_texture_binding(
            const types::HandleId handle) noexcept override;
        std::tuple<types::HandleId, types::ShaderBinding&> make_shader_binding(
            void) noexcept override;
        foundation::Option<const types::ShaderBinding&> get_shader_binding(
            const types::HandleId handle) const noexcept override;
        foundation::Option<types::ShaderBinding&> get_shader_binding(
            const types::HandleId handle) noexcept override;
        std::tuple<types::HandleId, types::ViewBinding&> make_view_binding(void) noexcept override;
        foundation::Option<const types::ViewBinding&> get_view_binding(
            const types::HandleId handle) const noexcept override;
        foundation::Option<types::ViewBinding&> get_view_binding(
            const types::HandleId handle) noexcept override;
        std::tuple<types::HandleId, types::StateBinding&> make_state_binding(
            void) noexcept override;
        foundation::Option<const types::StateBinding&> get_state_binding(
            const types::HandleId handle) const noexcept override;
        foundation::Option<types::StateBinding&> get_state_binding(
            const types::HandleId handle) noexcept override;
    };
} // namespace enishi::renderer
