#pragma once
#include "interface_shader_accessor.h"
#include "shader_refrection.h"
#include <d3d11.h>
#include <engine_types/assets/shader/shader_kind.h>
#include <renderer/errors/errors.h>
#include <variant>
#include <vector>

namespace enishi::renderer::directx {
    class ShaderPool : public IShaderAccessor {
      private:
        using Shaders =
            std::variant<std::monostate, VertexShader, PixelShader, ComputeShader, HullShader>;

      private:
        std::vector<Shaders> shaders;
        std::vector<Shaders> shader_reflections;

      public:
        std::tuple<std::size_t, VertexShader&> make_vertex_shader(void) noexcept override;
        std::tuple<std::size_t, PixelShader&> make_pixel_shader(void) noexcept override;
        std::tuple<std::size_t, ComputeShader&> make_compute_shader(void) noexcept override;
        std::tuple<std::size_t, HullShader&> make_hull_shader(void) noexcept override;
        void remove_shader(
            const types::ShaderKind shader_kind, const std::size_t index) noexcept override;
        foundation::Option<VertexShader&> get_vertex_shader(
            const std::size_t index) noexcept override;
        foundation::Option<const VertexShader&> get_vertex_shader(
            const std::size_t index) const noexcept override;
        foundation::Option<PixelShader&> get_pixel_shader(
            const std::size_t index) noexcept override;
        foundation::Option<const PixelShader&> get_pixel_shader(
            const std::size_t index) const noexcept override;
        foundation::Option<ComputeShader&> get_compute_shader(
            const std::size_t index) noexcept override;
        foundation::Option<const ComputeShader&> get_compute_shader(
            const std::size_t index) const noexcept override;
        foundation::Option<HullShader&> get_hull_shader(const std::size_t index) noexcept override;
        foundation::Option<const HullShader&> get_hull_shader(
            const std::size_t index) const noexcept override;
    };
} // namespace enishi::renderer::directx