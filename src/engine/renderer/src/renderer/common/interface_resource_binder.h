#pragma once
#include "binding_description.h"
#include <engine_types/handle/renderer/render_handle.h>
#include <foundation/option/option.h>

namespace enishi::renderer {
    class IResourceBinder {
      public:
        virtual ~IResourceBinder(void) noexcept = default;

        [[nodiscard]] virtual std::tuple<types::HandleId, DrawBinding&> make_draw_binding(
            void) noexcept = 0;
        [[nodiscard]] virtual foundation::Option<const DrawBinding&> get_draw_binding(
            const types::HandleId handle) const noexcept = 0;
        [[nodiscard]] virtual foundation::Option<DrawBinding&> get_draw_binding(
            const types::HandleId handle) noexcept = 0;

        [[nodiscard]] virtual std::tuple<types::HandleId, BufferBinding&> make_buffer_binding(
            void) noexcept = 0;
        [[nodiscard]] virtual foundation::Option<const BufferBinding&> get_buffer_binding(
            const types::HandleId handle) const noexcept = 0;
        [[nodiscard]] virtual foundation::Option<BufferBinding&> get_buffer_binding(
            const types::HandleId handle) noexcept = 0;

        [[nodiscard]] virtual std::tuple<types::HandleId, TextureBinding&> make_texture_binding(
            void) noexcept = 0;
        [[nodiscard]] virtual foundation::Option<const TextureBinding&> get_texture_binding(
            const types::HandleId handle) const noexcept = 0;
        [[nodiscard]] virtual foundation::Option<TextureBinding&> get_texture_binding(
            const types::HandleId handle) noexcept = 0;

        [[nodiscard]] virtual std::tuple<types::HandleId, ShaderBinding&> make_shader_binding(
            void) noexcept = 0;
        [[nodiscard]] virtual foundation::Option<const ShaderBinding&> get_shader_binding(
            const types::HandleId handle) const noexcept = 0;
        [[nodiscard]] virtual foundation::Option<ShaderBinding&> get_shader_binding(
            const types::HandleId handle) noexcept = 0;

        [[nodiscard]] virtual std::tuple<types::HandleId, ViewBinding&> make_view_binding(
            void) noexcept = 0;
        [[nodiscard]] virtual foundation::Option<const ViewBinding&> get_view_binding(
            const types::HandleId handle) const noexcept = 0;
        [[nodiscard]] virtual foundation::Option<ViewBinding&> get_view_binding(
            const types::HandleId handle) noexcept = 0;
    };
} // namespace enishi::renderer
