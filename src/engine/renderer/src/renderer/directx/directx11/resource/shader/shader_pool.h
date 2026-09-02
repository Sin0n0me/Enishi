#pragma once
#include "interface_native_shader_accessor.h"
#include "shader_refrection.h"
#include <engine_types/assets/shader/shader_kind.h>
#include <engine_types/handle/renderer/handles/resource_handles.h>
#include <platform/renderer/shader/interface_shader_accessor.h>
#include <renderer/common/resource_pool.h>

namespace enishi::renderer::directx {
    class ShaderPool : public INativeShaderAccessor, public platform::IShaderAccessor {
      private:
        struct ShaderHandle {
            types::ShaderKind kind;
            std::size_t resource_index;
            std::size_t reflection_index;
        };

      private:
        types::ResourceMapper<ShaderHandle> handle_mapper;
        ResourcePool<NativeVertexShader> native_vertex_shaders;
        ResourcePool<NativePixelShader> native_pixel_shaders;
        ResourcePool<NativeComputeShader> native_compute_shaders;
        ResourcePool<NativeHullShader> native_hull_shaders;
        ResourcePool<ShaderReflection> shader_reflections;

      public:
        std::tuple<types::HandleId, NativeVertexShader&> make_native_vertex_shader(
            void) noexcept override;
        std::tuple<types::HandleId, NativePixelShader&> make_native_pixel_shader(
            void) noexcept override;
        std::tuple<types::HandleId, NativeComputeShader&> make_native_compute_shader(
            void) noexcept override;
        std::tuple<types::HandleId, NativeHullShader&> make_native_hull_shader(
            void) noexcept override;
        void remove_native_shader(const types::HandleId handle) noexcept override;
        foundation::Option<types::ShaderKind> get_shader_kind(
            const types::HandleId handle) const noexcept override;
        foundation::Option<NativeVertexShader&> get_native_vertex_shader(
            const types::HandleId handle) noexcept override;
        foundation::Option<const NativeVertexShader&> get_native_vertex_shader(
            const types::HandleId handle) const noexcept override;
        foundation::Option<NativePixelShader&> get_native_pixel_shader(
            const types::HandleId handle) noexcept override;
        foundation::Option<const NativePixelShader&> get_native_pixel_shader(
            const types::HandleId handle) const noexcept override;
        foundation::Option<NativeComputeShader&> get_native_compute_shader(
            const types::HandleId handle) noexcept override;
        foundation::Option<const NativeComputeShader&> get_native_compute_shader(
            const types::HandleId handle) const noexcept override;
        foundation::Option<NativeHullShader&> get_native_hull_shader(
            const types::HandleId handle) noexcept override;
        foundation::Option<const NativeHullShader&> get_native_hull_shader(
            const types::HandleId handle) const noexcept override;

      public:
        std::tuple<types::HandleId, ShaderReflection&> make_shader_reflection(
            ShaderReflection&& shader_reflection) noexcept override;
        foundation::Option<ShaderReflection&> get_shader_reflection(
            const types::HandleId handle) noexcept override;
        foundation::Option<const ShaderReflection&> get_shader_reflection(
            const types::HandleId handle) const noexcept override;
        void remove_shader_reflection(const types::HandleId handle) noexcept override;
    };
} // namespace enishi::renderer::directx