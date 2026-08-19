#pragma once
#include "interface_native_shader_accessor.h"
#include "shader_refrection.h"
#include <d3d11.h>
#include <engine_types/assets/shader/shader_kind.h>
#include <memory>
#include <renderer/common/resource_pool.h>
#include <variant>
#include <vector>

namespace enishi::renderer::directx {
    class ShaderPool : public INativeShaderAccessor {
      private:
        using ShaderVariant = std::variant<std::monostate,
            NativeVertexShader,
            NativePixelShader,
            NativeComputeShader,
            NativeHullShader>;

        struct ShaderInfo {
            ShaderVariant shader;
            types::ShaderKind kind;
        };

      private:
        ResourcePool<ShaderInfo> shaders;
        ResourcePool<std::unique_ptr<ShaderReflection>> shader_reflections;

      public:
        std::tuple<std::size_t, NativeVertexShader&> make_native_vertex_shader(void) noexcept override;
        std::tuple<std::size_t, NativePixelShader&> make_native_pixel_shader(void) noexcept override;
        std::tuple<std::size_t, NativeComputeShader&> make_native_compute_shader(void) noexcept override;
        std::tuple<std::size_t, NativeHullShader&> make_native_hull_shader(void) noexcept override;
        void remove_native_shader(
            const types::ShaderKind shader_kind, const std::size_t index) noexcept override;
        foundation::Option<types::ShaderKind> get_shader_kind(
            const std::size_t index) const noexcept override;
        foundation::Option<NativeVertexShader&> get_native_vertex_shader(
            const std::size_t index) noexcept override;
        foundation::Option<const NativeVertexShader&> get_native_vertex_shader(
            const std::size_t index) const noexcept override;
        foundation::Option<NativePixelShader&> get_native_pixel_shader(
            const std::size_t index) noexcept override;
        foundation::Option<const NativePixelShader&> get_native_pixel_shader(
            const std::size_t index) const noexcept override;
        foundation::Option<NativeComputeShader&> get_native_compute_shader(
            const std::size_t index) noexcept override;
        foundation::Option<const NativeComputeShader&> get_native_compute_shader(
            const std::size_t index) const noexcept override;
        foundation::Option<NativeHullShader&> get_native_hull_shader(
            const std::size_t index) noexcept override;
        foundation::Option<const NativeHullShader&> get_native_hull_shader(
            const std::size_t index) const noexcept override;
        foundation::Option<IShaderReflection<DirectXError>*> get_shader_reflection(
            const std::size_t index) noexcept override;
        foundation::Option<IShaderReflection<DirectXError>* const> get_shader_reflection(
            const std::size_t index) const noexcept override;
        std::tuple<std::size_t, IShaderReflection<DirectXError>*> make_shader_reflection(
            void) noexcept override;
        void remove_shader_reflection(const std::size_t index) noexcept override;
    };
} // namespace enishi::renderer::directx