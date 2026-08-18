#pragma once
#include <d3d11.h>
#include <foundation/option/option.h>
#include <wrl/client.h>

namespace enishi::renderer::directx {
    class IShaderAccessor {
      public:
        using VertexShader = Microsoft::WRL::ComPtr<ID3D11VertexShader>;
        using PixelShader = Microsoft::WRL::ComPtr<ID3D11PixelShader>;
        using ComputeShader = Microsoft::WRL::ComPtr<ID3D11ComputeShader>;
        using HullShader = Microsoft::WRL::ComPtr<ID3D11HullShader>;

      public:
        virtual ~IShaderAccessor(void) noexcept = default;

        [[nodiscard]] virtual std::tuple<std::size_t, VertexShader&> make_vertex_shader(
            void) noexcept = 0;
        [[nodiscard]] virtual std::tuple<std::size_t, PixelShader&> make_pixel_shader(
            void) noexcept = 0;
        [[nodiscard]] virtual std::tuple<std::size_t, ComputeShader&> make_compute_shader(
            void) noexcept = 0;
        [[nodiscard]] virtual std::tuple<std::size_t, HullShader&> make_hull_shader(
            void) noexcept = 0;
        [[nodiscard]] virtual void remove_shader(
            const types::ShaderKind shader_kind, const std::size_t index) noexcept = 0;

        [[nodiscard]] virtual foundation::Option<VertexShader&> get_vertex_shader(
            const std::size_t index) noexcept = 0;
        [[nodiscard]] virtual foundation::Option<const VertexShader&> get_vertex_shader(
            const std::size_t index) const noexcept = 0;
        [[nodiscard]] virtual foundation::Option<PixelShader&> get_pixel_shader(
            const std::size_t index) noexcept = 0;
        [[nodiscard]] virtual foundation::Option<const PixelShader&> get_pixel_shader(
            const std::size_t index) const noexcept = 0;
        [[nodiscard]] virtual foundation::Option<ComputeShader&> get_compute_shader(
            const std::size_t index) noexcept = 0;
        [[nodiscard]] virtual foundation::Option<const ComputeShader&> get_compute_shader(
            const std::size_t index) const noexcept = 0;
        [[nodiscard]] virtual foundation::Option<HullShader&> get_hull_shader(
            const std::size_t index) noexcept = 0;
        [[nodiscard]] virtual foundation::Option<const HullShader&> get_hull_shader(
            const std::size_t index) const noexcept = 0;
    };
} // namespace enishi::renderer::directx