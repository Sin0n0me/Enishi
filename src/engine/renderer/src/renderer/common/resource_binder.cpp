#include "resource_binder.h"

namespace enishi::renderer {
    std::tuple<std::size_t, DrawBinding&> ResourceBinder::make_draw_binding(void) noexcept {
        return this->draw_bindings.make();
    }

    foundation::Option<const DrawBinding&> ResourceBinder::get_draw_binding(
        const std::size_t index) const noexcept {
        return this->draw_bindings.get(index);
    }

    std::tuple<std::size_t, BufferBinding&> ResourceBinder::make_buffer_binding(void) noexcept {
        return this->buffer_bindings.make();
    }

    foundation::Option<const BufferBinding&> ResourceBinder::get_buffer_binding(
        const std::size_t index) const noexcept {
        return this->buffer_bindings.get(index);
    }

    std::tuple<std::size_t, TextureBinding&> ResourceBinder::make_texture_binding(void) noexcept {
        return this->texture_bindings.make();
    }

    foundation::Option<const TextureBinding&> ResourceBinder::get_texture_binding(
        const std::size_t index) const noexcept {
        return this->texture_bindings.get(index);
    }
} // namespace enishi::renderer