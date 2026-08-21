#include "resource_binder.h"

namespace enishi::renderer {
    std::tuple<types::HandleId, DrawBinding&> ResourceBinder::make_draw_binding(void) noexcept {
        return this->handle_mapper.make_from(
            this->draw_bindings.make(), [](const std::size_t index) {
                return BindHandle{
                    .resource_index = index,
                };
            });
    }

    foundation::Option<const DrawBinding&> ResourceBinder::get_draw_binding(
        const types::HandleId handle) const noexcept {
        return this->handle_mapper.get(handle).and_then([this](const BindHandle handle) {
            return this->draw_bindings.get(handle.resource_index);
        });
    }

    std::tuple<types::HandleId, BufferBinding&> ResourceBinder::make_buffer_binding(void) noexcept {
        return this->handle_mapper.make_from(
            this->buffer_bindings.make(), [](const std::size_t index) {
                return BindHandle{
                    .resource_index = index,
                };
            });
    }

    foundation::Option<const BufferBinding&> ResourceBinder::get_buffer_binding(
        const types::HandleId handle) const noexcept {
        return this->handle_mapper.get(handle).and_then([this](const BindHandle handle) {
            return this->buffer_bindings.get(handle.resource_index);
        });
    }

    std::tuple<types::HandleId, TextureBinding&> ResourceBinder::make_texture_binding(
        void) noexcept {
        return this->handle_mapper.make_from(
            this->texture_bindings.make(), [](const std::size_t index) {
                return BindHandle{
                    .resource_index = index,
                };
            });
    }

    foundation::Option<const TextureBinding&> ResourceBinder::get_texture_binding(
        const types::HandleId handle) const noexcept {
        return this->handle_mapper.get(handle).and_then([this](const BindHandle handle) {
            return this->texture_bindings.get(handle.resource_index);
        });
    }
} // namespace enishi::renderer