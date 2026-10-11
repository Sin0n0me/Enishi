#pragma once
#include <engine_types/handle/renderer/render_handle.h>
#include <engine_types/renderer/binding_description.h>
#include <foundation/option/option.h>

namespace enishi::platform {
    class IResourceBinder {
      public:
        virtual ~IResourceBinder(void) noexcept = default;

        [[nodiscard]] virtual std::tuple<types::HandleId, types::DrawBinding&> make_draw_binding(
            void) noexcept = 0;
        [[nodiscard]] virtual foundation::Option<const types::DrawBinding&> get_draw_binding(
            const types::HandleId handle) const noexcept = 0;
        [[nodiscard]] virtual foundation::Option<types::DrawBinding&> get_draw_binding(
            const types::HandleId handle) noexcept = 0;

        [[nodiscard]] virtual std::tuple<types::HandleId, types::BufferBinding&>
        make_buffer_binding(void) noexcept = 0;
        [[nodiscard]] virtual foundation::Option<const types::BufferBinding&> get_buffer_binding(
            const types::HandleId handle) const noexcept = 0;
        [[nodiscard]] virtual foundation::Option<types::BufferBinding&> get_buffer_binding(
            const types::HandleId handle) noexcept = 0;

        [[nodiscard]] virtual std::tuple<types::HandleId, types::TextureBinding&>
        make_image_binding(void) noexcept = 0;
        [[nodiscard]] virtual foundation::Option<const types::TextureBinding&> get_texture_binding(
            const types::HandleId handle) const noexcept = 0;
        [[nodiscard]] virtual foundation::Option<types::TextureBinding&> get_texture_binding(
            const types::HandleId handle) noexcept = 0;

        [[nodiscard]] virtual std::tuple<types::HandleId, types::ShaderBinding&>
        make_shader_binding(void) noexcept = 0;
        [[nodiscard]] virtual foundation::Option<const types::ShaderBinding&> get_shader_binding(
            const types::HandleId handle) const noexcept = 0;
        [[nodiscard]] virtual foundation::Option<types::ShaderBinding&> get_shader_binding(
            const types::HandleId handle) noexcept = 0;

        [[nodiscard]] virtual std::tuple<types::HandleId, types::ViewBinding&> make_view_binding(
            void) noexcept = 0;
        [[nodiscard]] virtual foundation::Option<const types::ViewBinding&> get_view_binding(
            const types::HandleId handle) const noexcept = 0;
        [[nodiscard]] virtual foundation::Option<types::ViewBinding&> get_view_binding(
            const types::HandleId handle) noexcept = 0;

        [[nodiscard]] virtual std::tuple<types::HandleId, types::StateBinding&> make_state_binding(
            void) noexcept = 0;
        [[nodiscard]] virtual foundation::Option<const types::StateBinding&> get_state_binding(
            const types::HandleId handle) const noexcept = 0;
        [[nodiscard]] virtual foundation::Option<types::StateBinding&> get_state_binding(
            const types::HandleId handle) noexcept = 0;
    };
} // namespace enishi::platform
