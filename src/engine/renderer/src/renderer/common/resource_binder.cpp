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

    foundation::Option<DrawBinding&> ResourceBinder::get_draw_binding(
        const types::HandleId handle) noexcept {
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

    foundation::Option<BufferBinding&> ResourceBinder::get_buffer_binding(
        const types::HandleId handle) noexcept {
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

    foundation::Option<TextureBinding&> ResourceBinder::get_texture_binding(
        const types::HandleId handle) noexcept {
        return this->handle_mapper.get(handle).and_then([this](const BindHandle handle) {
            return this->texture_bindings.get(handle.resource_index);
        });
    }

    std::tuple<types::HandleId, ShaderBinding&> ResourceBinder::make_shader_binding(void) noexcept {
        return this->handle_mapper.make_from(
            this->shader_bindings.make(), [](const std::size_t index) {
                return BindHandle{
                    .resource_index = index,
                };
            });
    }

    foundation::Option<const ShaderBinding&> ResourceBinder::get_shader_binding(
        const types::HandleId handle) const noexcept {
        return this->handle_mapper.get(handle).and_then([this](const BindHandle handle) {
            return this->shader_bindings.get(handle.resource_index);
        });
    }

    foundation::Option<ShaderBinding&> ResourceBinder::get_shader_binding(
        const types::HandleId handle) noexcept {
        return this->handle_mapper.get(handle).and_then([this](const BindHandle handle) {
            return this->shader_bindings.get(handle.resource_index);
        });
    }

    std::tuple<types::HandleId, ViewBinding&> ResourceBinder::make_view_binding(void) noexcept {
        return this->handle_mapper.make_from(
            this->view_bindings.make(), [](const std::size_t index) {
                return BindHandle{
                    .resource_index = index,
                };
            });
    }

    foundation::Option<const ViewBinding&> ResourceBinder::get_view_binding(
        const types::HandleId handle) const noexcept {
        return this->handle_mapper.get(handle).and_then([this](const BindHandle handle) {
            return this->view_bindings.get(handle.resource_index);
        });
    }

    foundation::Option<ViewBinding&> ResourceBinder::get_view_binding(
        const types::HandleId handle) noexcept {
        return this->handle_mapper.get(handle).and_then([this](const BindHandle handle) {
            return this->view_bindings.get(handle.resource_index);
        });
    }
} // namespace enishi::renderer