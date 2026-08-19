#include "shader_pool.h"

namespace enishi::renderer::directx {
    std::tuple<std::size_t, ShaderPool::NativeVertexShader&> ShaderPool::make_native_vertex_shader(
        void) noexcept {
        auto [index, info] = this->shaders.emplace(ShaderInfo{
            .shader = NativeVertexShader{},
            .kind = types::ShaderKind::Vertex,
        });
        return {index, std::get<NativeVertexShader>(info.shader)};
    }

    std::tuple<std::size_t, ShaderPool::NativePixelShader&> ShaderPool::make_native_pixel_shader(void) noexcept {
        auto [index, info] = this->shaders.emplace(ShaderInfo{
            .shader = NativePixelShader{},
            .kind = types::ShaderKind::Vertex,
        });
        return {index, std::get<NativePixelShader>(info.shader)};
    }

    std::tuple<std::size_t, ShaderPool::NativeComputeShader&> ShaderPool::make_native_compute_shader(
        void) noexcept {
        auto [index, info] = this->shaders.emplace(ShaderInfo{
            .shader = NativeComputeShader{},
            .kind = types::ShaderKind::Vertex,
        });
        return {index, std::get<NativeComputeShader>(info.shader)};
    }

    std::tuple<std::size_t, ShaderPool::NativeHullShader&> ShaderPool::make_native_hull_shader(void) noexcept {
        auto [index, info] = this->shaders.emplace(ShaderInfo{
            .shader = NativeHullShader{},
            .kind = types::ShaderKind::Vertex,
        });
        return {index, std::get<NativeHullShader>(info.shader)};
    }

    void ShaderPool::remove_native_shader(
        const types::ShaderKind shader_kind, const std::size_t index) noexcept {
        auto opt = this->shaders.get(index);
        if (opt.is_none()) {
            return;
        }

        auto& shader = opt.unwrap_mut().shader;
        if (std::holds_alternative<NativeVertexShader>(shader) &&
            shader_kind == types::ShaderKind::Vertex) {
            shader = std::monostate{};
        } else if (std::holds_alternative<NativePixelShader>(shader) &&
                   shader_kind == types::ShaderKind::Pixel) {
            shader = std::monostate{};
        } else if (std::holds_alternative<NativeComputeShader>(shader) &&
                   shader_kind == types::ShaderKind::Compute) {
            shader = std::monostate{};
        } else if (std::holds_alternative<NativeHullShader>(shader) &&
                   shader_kind == types::ShaderKind::Hull) {
            shader = std::monostate{};
        }
    }

    // TODO: Optionのmapメソッド

    foundation::Option<types::ShaderKind> ShaderPool::get_shader_kind(
        const std::size_t index) const noexcept {
        auto opt =
            this->shaders.get(index).transform([](const ShaderInfo& info) { return info.kind; });
        if (!opt.has_value()) {
            return {};
        }
        return opt.value();
    }

    foundation::Option<ShaderPool::NativeVertexShader&> ShaderPool::get_native_vertex_shader(
        const std::size_t index) noexcept {
        auto opt =
            this->shaders.get(index).transform([](const ShaderInfo& info) { return info.shader; });
        if (opt.has_value()) {
            return {};
        }
        auto shader = std::get_if<NativeVertexShader>(&opt.value());
        if (!bool(shader)) {
        }
        return *shader;
    }

    foundation::Option<const ShaderPool::NativeVertexShader&> ShaderPool::get_native_vertex_shader(
        const std::size_t index) const noexcept {
        auto opt =
            this->shaders.get(index).transform([](const ShaderInfo& info) { return info.shader; });
        if (opt.has_value()) {
            return {};
        }
        auto shader = std::get_if<NativeVertexShader>(&opt.value());
        if (!bool(shader)) {
        }
        return *shader;
    }

    foundation::Option<ShaderPool::NativePixelShader&> ShaderPool::get_native_pixel_shader(
        const std::size_t index) noexcept {
        auto opt =
            this->shaders.get(index).transform([](const ShaderInfo& info) { return info.shader; });
        if (opt.has_value()) {
            return {};
        }
        auto shader = std::get_if<NativePixelShader>(&opt.value());
        if (!bool(shader)) {
        }
        return *shader;
    }

    foundation::Option<const ShaderPool::NativePixelShader&> ShaderPool::get_native_pixel_shader(
        const std::size_t index) const noexcept {
        auto opt =
            this->shaders.get(index).transform([](const ShaderInfo& info) { return info.shader; });
        if (opt.has_value()) {
            return {};
        }
        auto shader = std::get_if<NativePixelShader>(&opt.value());
        if (!bool(shader)) {
        }
        return *shader;
    }

    foundation::Option<ShaderPool::NativeComputeShader&> ShaderPool::get_native_compute_shader(
        const std::size_t index) noexcept {
        auto opt =
            this->shaders.get(index).transform([](const ShaderInfo& info) { return info.shader; });
        if (opt.has_value()) {
            return {};
        }
        auto shader = std::get_if<NativeComputeShader>(&opt.value());
        if (!bool(shader)) {
        }
        return *shader;
    }

    foundation::Option<const ShaderPool::NativeComputeShader&> ShaderPool::get_native_compute_shader(
        const std::size_t index) const noexcept {
        auto opt =
            this->shaders.get(index).transform([](const ShaderInfo& info) { return info.shader; });
        if (opt.has_value()) {
            return {};
        }
        auto shader = std::get_if<NativeComputeShader>(&opt.value());
        if (!bool(shader)) {
        }
        return *shader;
    }

    foundation::Option<ShaderPool::NativeHullShader&> ShaderPool::get_native_hull_shader(
        const std::size_t index) noexcept {
        auto opt =
            this->shaders.get(index).transform([](const ShaderInfo& info) { return info.shader; });
        if (opt.has_value()) {
            return {};
        }
        auto shader = std::get_if<NativeHullShader>(&opt.value());
        if (!bool(shader)) {
        }
        return *shader;
    }

    foundation::Option<const ShaderPool::NativeHullShader&> ShaderPool::get_native_hull_shader(
        const std::size_t index) const noexcept {
        auto opt =
            this->shaders.get(index).transform([](const ShaderInfo& info) { return info.shader; });
        if (opt.has_value()) {
            return {};
        }
        auto shader = std::get_if<NativeHullShader>(&opt.value());
        if (!bool(shader)) {
        }
        return *shader;
    }

    foundation::Option<IShaderReflection<DirectXError>*> ShaderPool::get_shader_reflection(
        const std::size_t index) noexcept {
        auto opt = this->shader_reflections.get(index);
        if (opt.has_value()) {
            return {};
        }
        auto& reflecttion = opt.unwrap_mut();
        if (!bool(reflecttion)) {
        }
        return reflecttion.get();
    }

    foundation::Option<IShaderReflection<DirectXError>* const> ShaderPool::get_shader_reflection(
        const std::size_t index) const noexcept {
        auto opt = this->shader_reflections.get(index);
        if (opt.has_value()) {
            return {};
        }
        auto& reflecttion = opt.unwrap();
        if (!bool(reflecttion)) {
        }
        return reflecttion.get();
    }

    std::tuple<std::size_t, IShaderReflection<DirectXError>*> ShaderPool::make_shader_reflection(
        void) noexcept {
        auto [index, reflection] =
            this->shader_reflections.emplace(std::make_unique<ShaderReflection>());
        return {index, reflection.get()};
    }

    void ShaderPool::remove_shader_reflection(const std::size_t index) noexcept {
        auto opt = this->shader_reflections.get(index);
        if (opt.is_none()) {
            return;
        }
        opt.reset();
    }
} // namespace enishi::renderer::directx