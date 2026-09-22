#include "resource_binder.h"

namespace enishi::renderer {
    std::tuple<types::HandleId, types::DrawBinding&> ResourceBinder::make_draw_binding(
        void) noexcept {
        return this->handle_mapper.make_from(
            this->draw_bindings.make(), [](const std::size_t index) {
                return BindHandle{
                    .resource_index = index,
                };
            });
    }

    foundation::Option<const types::DrawBinding&> ResourceBinder::get_draw_binding(
        const types::HandleId handle) const noexcept {
        return this->handle_mapper.get(handle).and_then([this](const BindHandle handle) {
            return this->draw_bindings.get(handle.resource_index);
        });
    }

    foundation::Option<types::DrawBinding&> ResourceBinder::get_draw_binding(
        const types::HandleId handle) noexcept {
        return this->handle_mapper.get(handle).and_then([this](const BindHandle handle) {
            return this->draw_bindings.get(handle.resource_index);
        });
    }

    std::tuple<types::HandleId, types::BufferBinding&> ResourceBinder::make_buffer_binding(
        void) noexcept {
        return this->handle_mapper.make_from(
            this->buffer_bindings.make(), [](const std::size_t index) {
                return BindHandle{
                    .resource_index = index,
                };
            });
    }

    foundation::Option<const types::BufferBinding&> ResourceBinder::get_buffer_binding(
        const types::HandleId handle) const noexcept {
        return this->handle_mapper.get(handle).and_then([this](const BindHandle handle) {
            return this->buffer_bindings.get(handle.resource_index);
        });
    }

    foundation::Option<types::BufferBinding&> ResourceBinder::get_buffer_binding(
        const types::HandleId handle) noexcept {
        return this->handle_mapper.get(handle).and_then([this](const BindHandle handle) {
            return this->buffer_bindings.get(handle.resource_index);
        });
    }

    std::tuple<types::HandleId, types::TextureBinding&> ResourceBinder::make_image_binding(
        void) noexcept {
        return this->handle_mapper.make_from(
            this->texture_bindings.make(), [](const std::size_t index) {
                return BindHandle{
                    .resource_index = index,
                };
            });
    }

    foundation::Option<const types::TextureBinding&> ResourceBinder::get_texture_binding(
        const types::HandleId handle) const noexcept {
        return this->handle_mapper.get(handle).and_then([this](const BindHandle handle) {
            return this->texture_bindings.get(handle.resource_index);
        });
    }

    foundation::Option<types::TextureBinding&> ResourceBinder::get_texture_binding(
        const types::HandleId handle) noexcept {
        return this->handle_mapper.get(handle).and_then([this](const BindHandle handle) {
            return this->texture_bindings.get(handle.resource_index);
        });
    }

    std::tuple<types::HandleId, types::ShaderBinding&> ResourceBinder::make_shader_binding(
        void) noexcept {
        return this->handle_mapper.make_from(
            this->shader_bindings.make(), [](const std::size_t index) {
                return BindHandle{
                    .resource_index = index,
                };
            });
    }

    foundation::Option<const types::ShaderBinding&> ResourceBinder::get_shader_binding(
        const types::HandleId handle) const noexcept {
        return this->handle_mapper.get(handle).and_then([this](const BindHandle handle) {
            return this->shader_bindings.get(handle.resource_index);
        });
    }

    foundation::Option<types::ShaderBinding&> ResourceBinder::get_shader_binding(
        const types::HandleId handle) noexcept {
        return this->handle_mapper.get(handle).and_then([this](const BindHandle handle) {
            return this->shader_bindings.get(handle.resource_index);
        });
    }

    std::tuple<types::HandleId, types::ViewBinding&> ResourceBinder::make_view_binding(
        void) noexcept {
        return this->handle_mapper.make_from(
            this->view_bindings.make(), [](const std::size_t index) {
                return BindHandle{
                    .resource_index = index,
                };
            });
    }

    foundation::Option<const types::ViewBinding&> ResourceBinder::get_view_binding(
        const types::HandleId handle) const noexcept {
        return this->handle_mapper.get(handle).and_then([this](const BindHandle handle) {
            return this->view_bindings.get(handle.resource_index);
        });
    }

    foundation::Option<types::ViewBinding&> ResourceBinder::get_view_binding(
        const types::HandleId handle) noexcept {
        return this->handle_mapper.get(handle).and_then([this](const BindHandle handle) {
            return this->view_bindings.get(handle.resource_index);
        });
    }

    std::tuple<types::HandleId, types::StateBinding&> ResourceBinder::make_state_binding(
        void) noexcept {
        return this->handle_mapper.make_from(
            this->state_bindings.make(), [](const std::size_t index) {
                return BindHandle{
                    .resource_index = index,
                };
            });
    }

    foundation::Option<const types::StateBinding&> ResourceBinder::get_state_binding(
        const types::HandleId handle) const noexcept {
        return this->handle_mapper.get(handle).and_then([this](const BindHandle handle) {
            return this->state_bindings.get(handle.resource_index);
        });
    }

    foundation::Option<types::StateBinding&> ResourceBinder::get_state_binding(
        const types::HandleId handle) noexcept {
        return this->handle_mapper.get(handle).and_then([this](const BindHandle handle) {
            return this->state_bindings.get(handle.resource_index);
        });
    }
} // namespace enishi::renderer