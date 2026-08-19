#pragma once
#include <d3d11.h>
#include <foundation/option/option.h>
#include <renderer/common/interface_shader_reflection.h>
#include <renderer/errors/errors.h>
#include <wrl/client.h>

namespace enishi::renderer::directx {
    class INativeShaderAccessor {
      public:
        using NativeVertexShader = Microsoft::WRL::ComPtr<ID3D11VertexShader>;
        using NativePixelShader = Microsoft::WRL::ComPtr<ID3D11PixelShader>;
        using NativeComputeShader = Microsoft::WRL::ComPtr<ID3D11ComputeShader>;
        using NativeHullShader = Microsoft::WRL::ComPtr<ID3D11HullShader>;

      public:
        virtual ~INativeShaderAccessor(void) noexcept = default;

        [[nodiscard]] virtual std::tuple<std::size_t, IShaderReflection<DirectXError>*>
        make_shader_reflection(void) noexcept = 0;
        [[nodiscard]] virtual std::tuple<std::size_t, NativeVertexShader&>
        make_native_vertex_shader(void) noexcept = 0;
        [[nodiscard]] virtual std::tuple<std::size_t, NativePixelShader&> make_native_pixel_shader(
            void) noexcept = 0;
        [[nodiscard]] virtual std::tuple<std::size_t, NativeComputeShader&>
        make_native_compute_shader(void) noexcept = 0;
        [[nodiscard]] virtual std::tuple<std::size_t, NativeHullShader&> make_native_hull_shader(
            void) noexcept = 0;
        [[nodiscard]] virtual void remove_native_shader(
            const types::ShaderKind shader_kind, const std::size_t index) noexcept = 0;
        [[nodiscard]] virtual void remove_shader_reflection(const std::size_t index) noexcept = 0;

        [[nodiscard]] virtual foundation::Option<types::ShaderKind> get_shader_kind(
            const std::size_t index) const noexcept = 0;

        [[nodiscard]] virtual foundation::Option<NativeVertexShader&> get_native_vertex_shader(
            const std::size_t index) noexcept = 0;
        [[nodiscard]] virtual foundation::Option<const NativeVertexShader&>
        get_native_vertex_shader(const std::size_t index) const noexcept = 0;
        [[nodiscard]] virtual foundation::Option<NativePixelShader&> get_native_pixel_shader(
            const std::size_t index) noexcept = 0;
        [[nodiscard]] virtual foundation::Option<const NativePixelShader&> get_native_pixel_shader(
            const std::size_t index) const noexcept = 0;
        [[nodiscard]] virtual foundation::Option<NativeComputeShader&> get_native_compute_shader(
            const std::size_t index) noexcept = 0;
        [[nodiscard]] virtual foundation::Option<const NativeComputeShader&>
        get_native_compute_shader(const std::size_t index) const noexcept = 0;
        [[nodiscard]] virtual foundation::Option<NativeHullShader&> get_native_hull_shader(
            const std::size_t index) noexcept = 0;
        [[nodiscard]] virtual foundation::Option<const NativeHullShader&> get_native_hull_shader(
            const std::size_t index) const noexcept = 0;

        [[nodiscard]] virtual foundation::Option<IShaderReflection<DirectXError>*>
        get_shader_reflection(const std::size_t index) noexcept = 0;
        [[nodiscard]] virtual foundation::Option<IShaderReflection<DirectXError>* const>
        get_shader_reflection(const std::size_t index) const noexcept = 0;
    };
} // namespace enishi::renderer::directx