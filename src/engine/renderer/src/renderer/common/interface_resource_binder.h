#pragma once
#include "binding_description.h"
#include <engine_types/handle/renderer/render_handle.h>
#include <foundation/option/option.h>

namespace enishi::renderer {
    class IResourceBinder {
      public:
        virtual ~IResourceBinder(void) noexcept = default;

        [[nodiscard]] virtual std::tuple<std::size_t, DrawBinding&> make_draw_binding(
            void) noexcept = 0;
        [[nodiscard]] virtual foundation::Option<const DrawBinding&> get_draw_binding(
            const std::size_t index) const noexcept = 0;

        [[nodiscard]] virtual std::tuple<std::size_t, BufferBinding&> make_buffer_binding(
            void) noexcept = 0;
        [[nodiscard]] virtual foundation::Option<const BufferBinding&> get_buffer_binding(
            const std::size_t index) const noexcept = 0;

        [[nodiscard]] virtual std::tuple<std::size_t, TextureBinding&> make_texture_binding(
            void) noexcept = 0;
        [[nodiscard]] virtual foundation::Option<const TextureBinding&> get_texture_binding(
            const std::size_t index) const noexcept = 0;
    };
} // namespace enishi::renderer
