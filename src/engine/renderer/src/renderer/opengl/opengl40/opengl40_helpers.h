#pragma once

#include <cstdint>
#include <engine_types/assets/texture/texture_data.h>
#include <engine_types/renderer/description/blend/detail/blend_factor.h>
#include <engine_types/renderer/description/blend/detail/blend_operator.h>
#include <engine_types/renderer/description/depth/detail/compare_operator.h>
#include <engine_types/renderer/description/sampler/detail/sampler_mode.h>
#include <engine_types/renderer/description/view/image_view_description.h>
#include <platform/renderer/interface_opengl_context.h>

namespace enishi::renderer::opengl::helpers {
    [[nodiscard]] bool load_gl40_functions(platform::IOpenGLContext& context);
    [[nodiscard]] std::uint32_t to_gl_compare(types::CompareOperator value);
    [[nodiscard]] std::uint32_t to_gl_blend_factor(types::BlendFactor value);
    [[nodiscard]] std::uint32_t to_gl_blend_operator(types::BlendOperator value);
    [[nodiscard]] std::uint32_t to_gl_address_mode(types::AddressMode value);
    [[nodiscard]] std::uint32_t to_gl_image_internal_format(types::ImageFormat value);
    [[nodiscard]] std::uint32_t to_gl_image_format(types::ImageFormat value);
    [[nodiscard]] std::uint32_t to_gl_image_type(types::ImageFormat value);
    [[nodiscard]] std::uint32_t to_gl_texture_internal_format(types::TextureFormat value);
    [[nodiscard]] std::uint32_t to_gl_texture_format(types::TextureFormat value);
    [[nodiscard]] std::uint32_t to_gl_texture_type(types::TextureFormat value);
} // namespace enishi::renderer::opengl::helpers
