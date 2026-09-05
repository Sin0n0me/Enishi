#include "shader_pool.h"

namespace enishi::renderer::directx {
    std::tuple<types::HandleId, ShaderPool::NativeVertexShader&>
    ShaderPool::make_native_vertex_shader(void) noexcept {
        return this->handle_mapper.make_from(
            this->native_vertex_shaders.make(), [](const std::size_t index) {
                return decltype(handle_mapper)::ValueType{
                    .kind = types::ShaderKind::Vertex,
                    .resource_index = index,
                };
            });
    }

    std::tuple<types::HandleId, ShaderPool::NativePixelShader&>
    ShaderPool::make_native_pixel_shader(void) noexcept {
        return this->handle_mapper.make_from(
            this->native_pixel_shaders.make(), [](const std::size_t index) {
                return decltype(handle_mapper)::ValueType{
                    .kind = types::ShaderKind::Pixel,
                    .resource_index = index,
                };
            });
    }

    std::tuple<types::HandleId, ShaderPool::NativeComputeShader&>
    ShaderPool::make_native_compute_shader(void) noexcept {
        return this->handle_mapper.make_from(
            this->native_compute_shaders.make(), [](const std::size_t index) {
                return decltype(handle_mapper)::ValueType{
                    .kind = types::ShaderKind::Compute,
                    .resource_index = index,
                };
            });
    }

    std::tuple<types::HandleId, ShaderPool::NativeHullShader&> ShaderPool::make_native_hull_shader(
        void) noexcept {
        return this->handle_mapper.make_from(
            this->native_hull_shaders.make(), [](const std::size_t index) {
                return decltype(handle_mapper)::ValueType{
                    .kind = types::ShaderKind::Hull,
                    .resource_index = index,
                };
            });
    }

    void ShaderPool::remove_native_shader(const types::HandleId handle) noexcept {
        const auto opt_shader = this->handle_mapper.get(handle);
        if (opt_shader.is_none()) {
            return;
        }

        const auto& native_handle = opt_shader.unwrap();
        switch (native_handle.kind) {
            case types::ShaderKind::Vertex: {
                this->native_vertex_shaders.get(native_handle.resource_index).unwrap_mut().Reset();
            } break;
            case types::ShaderKind::Pixel: {
                this->native_pixel_shaders.get(native_handle.resource_index).unwrap_mut().Reset();
            } break;
            case types::ShaderKind::Compute: {
                this->native_compute_shaders.get(native_handle.resource_index).unwrap_mut().Reset();
            } break;
            case types::ShaderKind::Hull: {
                this->native_hull_shaders.get(native_handle.resource_index).unwrap_mut().Reset();
            } break;
            default:
                return;
        }

        this->handle_mapper.remove(handle);
    }

    // TODO: Optionのmapメソッド

    foundation::Option<types::ShaderKind> ShaderPool::get_shader_kind(
        const types::HandleId handle) const noexcept {
        const auto opt_shader = this->handle_mapper.get(handle);
        if (opt_shader.is_none()) {
            return {};
        }
        return opt_shader.unwrap().kind;
    }

    foundation::Option<ShaderPool::NativeVertexShader&> ShaderPool::get_native_vertex_shader(
        const types::HandleId handle) noexcept {
        return this->handle_mapper.get(handle).and_then([this](const ShaderHandle& shader) {
            return this->native_vertex_shaders.get(shader.resource_index);
        });
    }

    foundation::Option<const ShaderPool::NativeVertexShader&> ShaderPool::get_native_vertex_shader(
        const types::HandleId handle) const noexcept {
        return this->handle_mapper.get(handle).and_then([this](const ShaderHandle& shader) {
            return this->native_vertex_shaders.get(shader.resource_index);
        });
    }

    foundation::Option<ShaderPool::NativePixelShader&> ShaderPool::get_native_pixel_shader(
        const types::HandleId handle) noexcept {
        return this->handle_mapper.get(handle).and_then([this](const ShaderHandle& shader) {
            return this->native_pixel_shaders.get(shader.resource_index);
        });
    }

    foundation::Option<const ShaderPool::NativePixelShader&> ShaderPool::get_native_pixel_shader(
        const types::HandleId handle) const noexcept {
        return this->handle_mapper.get(handle).and_then([this](const ShaderHandle& shader) {
            return this->native_pixel_shaders.get(shader.resource_index);
        });
    }

    foundation::Option<ShaderPool::NativeComputeShader&> ShaderPool::get_native_compute_shader(
        const types::HandleId handle) noexcept {
        return this->handle_mapper.get(handle).and_then([this](const ShaderHandle& shader) {
            return this->native_compute_shaders.get(shader.resource_index);
        });
    }

    foundation::Option<const ShaderPool::NativeComputeShader&>
    ShaderPool::get_native_compute_shader(const types::HandleId handle) const noexcept {
        return this->handle_mapper.get(handle).and_then([this](const ShaderHandle& shader) {
            return this->native_compute_shaders.get(shader.resource_index);
        });
    }

    foundation::Option<ShaderPool::NativeHullShader&> ShaderPool::get_native_hull_shader(
        const types::HandleId handle) noexcept {
        return this->handle_mapper.get(handle).and_then([this](const ShaderHandle& shader) {
            return this->native_hull_shaders.get(shader.resource_index);
        });
    }

    foundation::Option<const ShaderPool::NativeHullShader&> ShaderPool::get_native_hull_shader(
        const types::HandleId handle) const noexcept {
        return this->handle_mapper.get(handle).and_then([this](const ShaderHandle& shader) {
            return this->native_hull_shaders.get(shader.resource_index);
        });
    }

    std::tuple<types::HandleId, ShaderPool::ShaderReflection&> ShaderPool::make_shader_reflection(
        ShaderReflection&& shader_reflection) noexcept {
        return this->handle_mapper.make_from(
            this->shader_reflections.emplace(std::move(shader_reflection)),
            [](const std::size_t index) {
                return decltype(handle_mapper)::ValueType{
                    .kind = types::ShaderKind::Unknown,
                    .reflection_index = index,
                };
            });
    }

    foundation::Option<ShaderPool::ShaderReflection&> ShaderPool::get_shader_reflection(
        const types::HandleId handle) noexcept {
        return this->handle_mapper.get(handle).and_then(
            [this](
                const ShaderHandle& shader) -> foundation::Option<ShaderPool::ShaderReflection&> {
                auto opt_reflection = this->shader_reflections.get(shader.reflection_index);
                if (opt_reflection.is_none()) {
                    return {};
                }
                auto& reflection = opt_reflection.unwrap_mut();
                if (!bool(reflection)) {
                    return {};
                }
                return reflection;
            });
    }

    foundation::Option<const ShaderPool::ShaderReflection&> ShaderPool::get_shader_reflection(
        const types::HandleId handle) const noexcept {
        return this->handle_mapper.get(handle).and_then(
            [this](const ShaderHandle& shader)
                -> foundation::Option<const ShaderPool::ShaderReflection&> {
                auto opt_reflection = this->shader_reflections.get(shader.reflection_index);
                if (opt_reflection.is_none()) {
                    return {};
                }
                auto& reflection = opt_reflection.unwrap();
                if (!bool(reflection)) {
                    return {};
                }
                return reflection;
            });
    }

    void ShaderPool::remove_shader_reflection(const types::HandleId handle) noexcept {
        const auto opt_shader = this->handle_mapper.get(handle);
        if (opt_shader.is_none()) {
            return;
        }

        const auto& native_handle = opt_shader.unwrap();
        this->shader_reflections.get(native_handle.resource_index).unwrap_mut().reset();
    }
} // namespace enishi::renderer::directx