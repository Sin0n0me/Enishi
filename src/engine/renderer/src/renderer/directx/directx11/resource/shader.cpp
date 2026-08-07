#include "shader.h"

namespace enishi::renderer::directx {
    foundation::VoidResult<DirectXError> ShaderPool::create(
        const types::HandleId id, const types::ShaderKind shader_kind) noexcept {
        if (this->handle_to_shader.contains(id)) {
            return {};
        }

        switch (shader_kind) {
            case types::ShaderKind::Vertex: {
                this->handle_to_shader[id] = ShaderPool::VertexShader();
            } break;
            case types::ShaderKind::Pixel: {
                this->handle_to_shader[id] = ShaderPool::PixelShader();
            } break;
            case types::ShaderKind::Compute: {
                this->handle_to_shader[id] = ShaderPool::ComputeShader();
            } break;
            case types::ShaderKind::Hull: {
                this->handle_to_shader[id] = ShaderPool::HullShader();
            } break;
            default:
                return foundation::Error(DirectXError::ShaderError, "対応していないシェーダーです");
        }

        return {};
    }

    foundation::Option<types::ShaderKind> ShaderPool::get_shader_type(
        const types::HandleId id) const noexcept {
        const auto opt_shader = this->get_shader(id);
        if (opt_shader.is_none()) {
            return {};
        }
        if (std::get_if<VertexShader>(&opt_shader.unwrap())) {
            return types::ShaderKind::Vertex;
        }
        if (std::get_if<PixelShader>(&opt_shader.unwrap())) {
            return types::ShaderKind::Pixel;
        }
        if (std::get_if<ComputeShader>(&opt_shader.unwrap())) {
            return types::ShaderKind::Compute;
        }
        if (std::get_if<HullShader>(&opt_shader.unwrap())) {
            return types::ShaderKind::Hull;
        }

        return {};
    }

    foundation::Option<ShaderPool::VertexShader&> ShaderPool::get_vertex_shader(
        const types::HandleId id) noexcept {
        auto opt_shader = this->get_shader(id);
        if (opt_shader.is_none()) {
            return {};
        }
        if (auto shader = std::get_if<VertexShader>(&opt_shader.unwrap_mut())) {
            return *shader;
        }
        return {};
    }

    foundation::Option<const ShaderPool::VertexShader&> ShaderPool::get_vertex_shader(
        const types::HandleId id) const noexcept {
        auto opt_shader = this->get_shader(id);
        if (opt_shader.is_none()) {
            return {};
        }
        if (auto shader = std::get_if<VertexShader>(&opt_shader.unwrap())) {
            return *shader;
        }
        return {};
    }

    foundation::Option<ShaderPool::PixelShader&> ShaderPool::get_pixel_shader(
        const types::HandleId id) noexcept {
        auto opt_shader = this->get_shader(id);
        if (opt_shader.is_none()) {
            return {};
        }
        if (const auto shader = std::get_if<PixelShader>(&opt_shader.unwrap_mut())) {
            return *shader;
        }
        return {};
    }

    foundation::Option<const ShaderPool::PixelShader&> ShaderPool::get_pixel_shader(
        const types::HandleId id) const noexcept {
        const auto opt_shader = this->get_shader(id);
        if (opt_shader.is_none()) {
            return {};
        }
        if (const auto shader = std::get_if<PixelShader>(&opt_shader.unwrap())) {
            return *shader;
        }
        return {};
    }

    foundation::Option<ShaderPool::ComputeShader&> ShaderPool::get_compute_shader(
        const types::HandleId id) noexcept {
        auto opt_shader = this->get_shader(id);
        if (opt_shader.is_none()) {
            return {};
        }
        if (const auto shader = std::get_if<ComputeShader>(&opt_shader.unwrap_mut())) {
            return *shader;
        }
        return {};
    }

    foundation::Option<const ShaderPool::ComputeShader&> ShaderPool::get_compute_shader(
        const types::HandleId id) const noexcept {
        const auto opt_shader = this->get_shader(id);
        if (opt_shader.is_none()) {
            return {};
        }
        if (const auto shader = std::get_if<ComputeShader>(&opt_shader.unwrap())) {
            return *shader;
        }
        return {};
    }

    foundation::Option<ShaderPool::Shaders&> ShaderPool::get_shader(
        const types::HandleId id) noexcept {
        const auto& iter = this->handle_to_shader.find(id);
        if (iter == this->handle_to_shader.end()) {
            return {};
        }
        return iter->second;
    }

    foundation::Option<const ShaderPool::Shaders&> ShaderPool::get_shader(
        const types::HandleId id) const noexcept {
        const auto& iter = this->handle_to_shader.find(id);
        if (iter == this->handle_to_shader.end()) {
            return {};
        }
        return iter->second;
    }
} // namespace enishi::renderer::directx