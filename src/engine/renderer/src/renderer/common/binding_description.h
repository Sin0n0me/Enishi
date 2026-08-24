#pragma once
#include <cstdint>
#include <engine_types/assets/shader/shader_kind.h>
#include <engine_types/assets/texture/texture_data.h>
#include <foundation/option/option.h>
#include <variant>
#include <vector>

namespace enishi::renderer {
    enum class ResourceType : std::uint8_t {
        Buffer,
        Texture,
        Sampler,
        Shader,
        Pipeline,
    };

    enum class BindingType : std::uint8_t {
        UniformBuffer,
        StorageBuffer,
        Texture,
        StorageTexture,
        Sampler,
        CombinedTextureSampler,
    };

    struct VertexBufferParameter {
        std::uint32_t target;
        std::uint32_t stride;
        std::uint32_t offset;
    };

    struct IndexBufferParameter {
        std::uint32_t stride;
        std::uint32_t offset;
    };

    struct UniformBufferParameter {
        std::uint32_t target;
        types::ShaderKind target_shader;
    };

    struct BufferBinding {
        using BufferParameter =
            std::variant<VertexBufferParameter, IndexBufferParameter, UniformBufferParameter>;
        BufferParameter parameter;
    };

    struct TextureBinding {
        types::ShaderKind target_shader;
        std::uint32_t target;

        std::uint32_t mip_level;
        std::uint32_t mip_count;
        std::uint32_t array_slice;
        std::uint32_t array_count;
        // TextureViewType view_type;
    };

    struct SamplerBinding {};

    struct ShaderBinding {};

    struct DepthStencilParameter {};
    struct RenderTargetParameter {};
    struct ShaderResourceParameter {
        types::ShaderKind target_shader;
        std::uint32_t target;
    };
    struct UnorderedAccessParameter {
        std::uint32_t target;
        std::uint32_t stride;
        std::uint32_t offset;
    };

    struct ViewBinding {
        using ViewParameter = std::variant<DepthStencilParameter,
            RenderTargetParameter,
            ShaderResourceParameter,
            UnorderedAccessParameter>;
        ViewParameter parameter;

        foundation::Option<const DepthStencilParameter&> get_depth_stencil_param(
            void) const noexcept {
            if (auto p = std::get_if<DepthStencilParameter>(&this->parameter)) {
                return *p;
            }
            return {};
        }
        foundation::Option<DepthStencilParameter&> get_depth_stencil_param(void) noexcept {
            if (auto p = std::get_if<DepthStencilParameter>(&this->parameter)) {
                return *p;
            }
            return {};
        }
        foundation::Option<const RenderTargetParameter&> get_render_target_param(
            void) const noexcept {
            if (auto p = std::get_if<RenderTargetParameter>(&this->parameter)) {
                return *p;
            }
            return {};
        }
        foundation::Option<RenderTargetParameter&> get_render_target_param(void) noexcept {
            if (auto p = std::get_if<RenderTargetParameter>(&this->parameter)) {
                return *p;
            }
            return {};
        }
        foundation::Option<const ShaderResourceParameter&> get_shader_resource_param(
            void) const noexcept {
            if (auto p = std::get_if<ShaderResourceParameter>(&this->parameter)) {
                return *p;
            }
            return {};
        }
        foundation::Option<ShaderResourceParameter&> get_shader_resource_param(void) noexcept {
            if (auto p = std::get_if<ShaderResourceParameter>(&this->parameter)) {
                return *p;
            }
            return {};
        }
        foundation::Option<const UnorderedAccessParameter&> get_unodered_access_param(
            void) const noexcept {
            if (auto p = std::get_if<UnorderedAccessParameter>(&this->parameter)) {
                return *p;
            }
            return {};
        }
        foundation::Option<UnorderedAccessParameter&> get_unodered_access_param(void) noexcept {
            if (auto p = std::get_if<UnorderedAccessParameter>(&this->parameter)) {
                return *p;
            }
            return {};
        }
    };

    struct DrawParameter {
        std::uint32_t vertex_count;
        std::uint32_t instance_count;
        std::uint32_t first_vertex;
        std::uint32_t first_instance;
    };

    struct DrawIndexedParameter {
        std::uint32_t index_count;
        std::uint32_t instance_count;
        std::int32_t first_index;
        std::uint32_t vertex_offset;
        std::uint32_t first_instance;
    };

    struct DrawBinding {
        using DrawParameter = std::variant<DrawParameter, DrawIndexedParameter>;
        DrawParameter parameter;
    };

    struct BindingKey {
        std::uint32_t space;
        std::uint32_t binding;
    };
} // namespace enishi::renderer