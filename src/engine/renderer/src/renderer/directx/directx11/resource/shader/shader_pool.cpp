#include "shader_pool.h"

namespace enishi::renderer::directx {
    std::tuple<std::size_t, ShaderPool::VertexShader&> ShaderPool::make_vertex_shader(
        void) noexcept {
        const auto index = this->shaders.size();
        auto shader = VertexShader{};
        this->shaders.emplace_back(shader);
        return {index, shader};
    }

    std::tuple<std::size_t, ShaderPool::PixelShader&> ShaderPool::make_pixel_shader(void) noexcept {
        const auto index = this->shaders.size();
        auto shader = PixelShader{};
        this->shaders.emplace_back(shader);
        return {index, shader};
    }

    std::tuple<std::size_t, ShaderPool::ComputeShader&> ShaderPool::make_compute_shader(
        void) noexcept {
        const auto index = this->shaders.size();
        auto shader = ComputeShader{};
        this->shaders.emplace_back(shader);
        return {index, shader};
    }

    std::tuple<std::size_t, ShaderPool::HullShader&> ShaderPool::make_hull_shader(void) noexcept {
        const auto index = this->shaders.size();
        auto shader = HullShader{};
        this->shaders.emplace_back(shader);
        return {index, shader};
    }

    void ShaderPool::remove_shader(
        const types::ShaderKind shader_kind, const std::size_t index) noexcept {
        if (this->shaders.size() < index + 1) {
            return;
        }

        auto& shader = this->shaders.at(index);
        if (std::holds_alternative<VertexShader>(shader) &&
            shader_kind == types::ShaderKind::Vertex) {
            shader = std::monostate{};
        } else if (std::holds_alternative<PixelShader>(shader) &&
                   shader_kind == types::ShaderKind::Pixel) {
            shader = std::monostate{};
        } else if (std::holds_alternative<ComputeShader>(shader) &&
                   shader_kind == types::ShaderKind::Compute) {
            shader = std::monostate{};
        } else if (std::holds_alternative<HullShader>(shader) &&
                   shader_kind == types::ShaderKind::Hull) {
            shader = std::monostate{};
        }
    }

    foundation::Option<ShaderPool::VertexShader&> ShaderPool::get_vertex_shader(
        const std::size_t index) noexcept {
        if (this->shaders.size() < index + 1) {
            return {};
        }
        auto shader = std::get_if<VertexShader>(&this->shaders.at(index));
        if (!bool(shader)) {
            return {};
        }
        return *shader;
    }

    foundation::Option<const ShaderPool::VertexShader&> ShaderPool::get_vertex_shader(
        const std::size_t index) const noexcept {
        if (this->shaders.size() < index + 1) {
            return {};
        }
        auto shader = std::get_if<VertexShader>(&this->shaders.at(index));
        if (!bool(shader)) {
            return {};
        }
        return *shader;
    }

    foundation::Option<ShaderPool::PixelShader&> ShaderPool::get_pixel_shader(
        const std::size_t index) noexcept {
        if (this->shaders.size() < index + 1) {
            return {};
        }
        auto shader = std::get_if<PixelShader>(&this->shaders.at(index));
        if (!bool(shader)) {
            return {};
        }
        return *shader;
    }

    foundation::Option<const ShaderPool::PixelShader&> ShaderPool::get_pixel_shader(
        const std::size_t index) const noexcept {
        if (this->shaders.size() < index + 1) {
            return {};
        }
        auto shader = std::get_if<PixelShader>(&this->shaders.at(index));
        if (!bool(shader)) {
            return {};
        }
        return *shader;
    }

    foundation::Option<ShaderPool::ComputeShader&> ShaderPool::get_compute_shader(
        const std::size_t index) noexcept {
        if (this->shaders.size() < index + 1) {
            return {};
        }
        auto shader = std::get_if<ComputeShader>(&this->shaders.at(index));
        if (!bool(shader)) {
            return {};
        }
        return *shader;
    }

    foundation::Option<const ShaderPool::ComputeShader&> ShaderPool::get_compute_shader(
        const std::size_t index) const noexcept {
        if (this->shaders.size() < index + 1) {
            return {};
        }
        auto shader = std::get_if<ComputeShader>(&this->shaders.at(index));
        if (!bool(shader)) {
            return {};
        }
        return *shader;
    }

    foundation::Option<ShaderPool::HullShader&> ShaderPool::get_hull_shader(
        const std::size_t index) noexcept {
        if (this->shaders.size() < index + 1) {
            return {};
        }
        auto shader = std::get_if<HullShader>(&this->shaders.at(index));
        if (!bool(shader)) {
            return {};
        }
        return *shader;
    }

    foundation::Option<const ShaderPool::HullShader&> ShaderPool::get_hull_shader(
        const std::size_t index) const noexcept {
        if (this->shaders.size() < index + 1) {
            return {};
        }
        auto shader = std::get_if<HullShader>(&this->shaders.at(index));
        if (!bool(shader)) {
            return {};
        }
        return *shader;
    }
} // namespace enishi::renderer::directx