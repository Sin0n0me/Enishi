#include "opengl40_render_states.h"
#include <cstdint>
#include <glad/gl.h>
#include <renderer/opengl/opengl40/runtime/opengl40_handles.h>
#include <renderer/opengl/opengl40/support/opengl40_errors.h>
#include <renderer/opengl/opengl40/support/opengl40_helpers.h>

namespace enishi::renderer::opengl {
    namespace {
        constexpr std::uint32_t NO_GL_OBJECT = 0;
        constexpr std::uint32_t RESOURCE_COUNT = 1;
        constexpr std::uint32_t FIRST_RENDER_TARGET = 0;
        constexpr std::uint8_t EMPTY_COLOR_WRITE_MASK = 0;
        constexpr std::uint32_t GL_TEXTURE_MAX_ANISOTROPY_EXT_VALUE = 0x84FE;
    } // namespace
    platform::RenderResult<types::RenderHandle> OpenGL40RenderStates::create_rasterizer(
        OpenGL40RendererState& runtime,
        ResourceAccessor& resource_accessor,
        RenderHandleMapper& handle_mapper,
        const types::RasterizerStateDescription& state) {
        if (state.depth_bias.enable && state.depth_bias.clamp != 0.0f) {
            return foundation::Error(platform::RenderError::MakeError,
                "OpenGL 4.0 does not support depth bias clamping");
        }
        const auto handle =
            OpenGL40Handles::make_handle(handle_mapper, types::RenderHandleType::State);
        runtime.rasterizers.emplace(handle, state);
        resource_accessor.add_state(handle_mapper[handle].resource, types::StateKind::Rasterizer);
        return handle;
    }
    platform::RenderResult<types::RenderHandle> OpenGL40RenderStates::create_depth_stencil(
        OpenGL40RendererState& runtime,
        ResourceAccessor& resource_accessor,
        RenderHandleMapper& handle_mapper,
        const types::DepthStencilStateDescription& state) {
        const auto handle =
            OpenGL40Handles::make_handle(handle_mapper, types::RenderHandleType::State);
        runtime.depth_stencils.emplace(handle, state);
        resource_accessor.add_state(handle_mapper[handle].resource, types::StateKind::DepthStencil);
        return handle;
    }
    platform::RenderResult<types::RenderHandle> OpenGL40RenderStates::create_sampler(
        OpenGL40RendererState& runtime,
        ResourceAccessor& resource_accessor,
        RenderHandleMapper& handle_mapper,
        const types::SamplerStateDescription& state) {
        auto pending = check_gl_errors("before sampler creation");
        if (pending.is_err()) {
            return std::move(pending).take_err();
        }
        if (state.anisotropy != types::AnisotropyLevel::None && !helpers::supports_anisotropy()) {
            return foundation::Error(platform::RenderError::MakeError,
                "OpenGL texture anisotropy requires GL_EXT_texture_filter_anisotropic");
        }
        GLuint sampler = NO_GL_OBJECT;
        glGenSamplers(RESOURCE_COUNT, &sampler);
        glSamplerParameteri(sampler,
            GL_TEXTURE_MIN_FILTER,
            helpers::to_gl_min_filter(state.min_filter, state.mip_filter));
        glSamplerParameteri(sampler,
            GL_TEXTURE_MAG_FILTER,
            state.mag_filter == types::FilterMode::Nearest ? GL_NEAREST : GL_LINEAR);
        glSamplerParameteri(
            sampler, GL_TEXTURE_WRAP_S, helpers::to_gl_address_mode(state.address_u));
        glSamplerParameteri(
            sampler, GL_TEXTURE_WRAP_T, helpers::to_gl_address_mode(state.address_v));
        glSamplerParameteri(
            sampler, GL_TEXTURE_WRAP_R, helpers::to_gl_address_mode(state.address_w));
        glSamplerParameterf(sampler, GL_TEXTURE_MIN_LOD, state.min_lod);
        glSamplerParameterf(sampler, GL_TEXTURE_MAX_LOD, state.max_lod);
        glSamplerParameterf(sampler, GL_TEXTURE_LOD_BIAS, state.mip_lod_bias);
        if (state.anisotropy != types::AnisotropyLevel::None) {
            glSamplerParameterf(
                sampler, GL_TEXTURE_MAX_ANISOTROPY_EXT_VALUE, static_cast<float>(state.anisotropy));
        }
        auto result = check_gl_errors("sampler creation and configuration");
        if (result.is_err()) {
            glDeleteSamplers(RESOURCE_COUNT, &sampler);
            return std::move(result).take_err();
        }
        const auto handle =
            OpenGL40Handles::make_handle(handle_mapper, types::RenderHandleType::State);
        runtime.samplers.emplace(handle, state);
        runtime.sampler_objects.emplace(handle, sampler);
        runtime.gl_samplers.emplace_back(sampler);
        resource_accessor.add_state(handle_mapper[handle].resource, types::StateKind::Sampler);
        return handle;
    }
    platform::RenderResult<types::RenderHandle> OpenGL40RenderStates::create_blend(
        OpenGL40RendererState& runtime,
        ResourceAccessor& resource_accessor,
        RenderHandleMapper& handle_mapper,
        const types::BlendStateDescription& state) {
        if (state.alpha_to_coverage) {
            return foundation::Error(platform::RenderError::MakeError,
                "OpenGL 4.0 alpha-to-coverage requires unsupported multisample images");
        }
        if (state.independent_blend) {
            return foundation::Error(platform::RenderError::MakeError,
                "OpenGL 4.0 independent blending is not implemented");
        }
        const auto handle =
            OpenGL40Handles::make_handle(handle_mapper, types::RenderHandleType::State);
        runtime.blends.emplace(handle, state);
        resource_accessor.add_state(handle_mapper[handle].resource, types::StateKind::Blend);
        return handle;
    }
    void OpenGL40RenderStates::submit_command_state(
        OpenGL40RendererState& runtime, const types::DrawCommand& command) {
        if (command.sub_command != types::SubCommand::Bind) {
            return;
        }
        if (const auto it = runtime.rasterizers.find(command.handle);
            it != runtime.rasterizers.end()) {
            OpenGL40RenderStates::apply_rasterizer(it->second);
            return;
        }
        if (const auto it = runtime.depth_stencils.find(command.handle);
            it != runtime.depth_stencils.end()) {
            OpenGL40RenderStates::apply_depth_stencil(it->second);
            return;
        }
        if (const auto it = runtime.blends.find(command.handle); it != runtime.blends.end()) {
            OpenGL40RenderStates::apply_blend(it->second);
            return;
        }
        if (const auto it = runtime.samplers.find(command.handle); it != runtime.samplers.end()) {
            const auto sampler = runtime.sampler_objects.find(command.handle);
            if (sampler == runtime.sampler_objects.end()) {
                return;
            }
            runtime.active_sampler = sampler->second;
        }
    }
    void OpenGL40RenderStates::apply_rasterizer(const types::RasterizerStateDescription& state) {
        if (state.cull_mode == types::CullMode::None) {
            glDisable(GL_CULL_FACE);
        } else {
            glEnable(GL_CULL_FACE);
        }

        if (state.cull_mode == types::CullMode::Front) {
            glCullFace(GL_FRONT);
        } else {
            glCullFace(GL_BACK);
        }
        glPolygonMode(
            GL_FRONT_AND_BACK, state.fill_mode == types::FillMode::Wireframe ? GL_LINE : GL_FILL);
        glFrontFace(state.front_face == types::FrontFace::Clockwise ? GL_CW : GL_CCW);
        glLineWidth(state.line_width);
        if (state.depth_clamp) {
            glEnable(GL_DEPTH_CLAMP);
        } else {
            glDisable(GL_DEPTH_CLAMP);
        }
        if (state.depth_bias.enable) {
            glEnable(GL_POLYGON_OFFSET_FILL);
            glPolygonOffset(state.depth_bias.slope_factor, state.depth_bias.constant_factor);
        } else {
            glDisable(GL_POLYGON_OFFSET_FILL);
        }
    }
    void OpenGL40RenderStates::apply_depth_stencil(
        const types::DepthStencilStateDescription& state) {
        if (state.depth.enabled) {
            glEnable(GL_DEPTH_TEST);
        } else {
            glDisable(GL_DEPTH_TEST);
        }

        glDepthMask(state.depth.write_enabled ? GL_TRUE : GL_FALSE);
        glDepthFunc(helpers::to_gl_compare(state.depth.compare_operator));
        if (state.stencil.enabled) {
            glEnable(GL_STENCIL_TEST);
            glStencilMaskSeparate(GL_FRONT, state.stencil.front.write_mask);
            glStencilFuncSeparate(GL_FRONT,
                helpers::to_gl_compare(state.stencil.front.compare_operator),
                static_cast<GLint>(state.stencil.front.reference),
                state.stencil.front.compare_mask);
            glStencilOpSeparate(GL_FRONT,
                helpers::to_gl_stencil_operator(state.stencil.front.stencil_fail_operator),
                helpers::to_gl_stencil_operator(state.stencil.front.depth_fail_operator),
                helpers::to_gl_stencil_operator(state.stencil.front.pass_operator));
            glStencilMaskSeparate(GL_BACK, state.stencil.back.write_mask);
            glStencilFuncSeparate(GL_BACK,
                helpers::to_gl_compare(state.stencil.back.compare_operator),
                static_cast<GLint>(state.stencil.back.reference),
                state.stencil.back.compare_mask);
            glStencilOpSeparate(GL_BACK,
                helpers::to_gl_stencil_operator(state.stencil.back.stencil_fail_operator),
                helpers::to_gl_stencil_operator(state.stencil.back.depth_fail_operator),
                helpers::to_gl_stencil_operator(state.stencil.back.pass_operator));
        } else {
            glDisable(GL_STENCIL_TEST);
        }
    }
    void OpenGL40RenderStates::apply_blend(const types::BlendStateDescription& state) {
        const auto& target = state.render_targets[FIRST_RENDER_TARGET];

        target.enabled ? glEnable(GL_BLEND) : glDisable(GL_BLEND);
        glBlendFuncSeparate(helpers::to_gl_blend_factor(target.src_color),
            helpers::to_gl_blend_factor(target.dst_color),
            helpers::to_gl_blend_factor(target.src_alpha),
            helpers::to_gl_blend_factor(target.dst_alpha));
        glBlendEquationSeparate(helpers::to_gl_blend_operator(target.color_operator),
            helpers::to_gl_blend_operator(target.alpha_operator));
        glColorMask(
            (target.write_mask & static_cast<std::uint8_t>(types::ColorWriteMask::ColorWriteR)) !=
                EMPTY_COLOR_WRITE_MASK,
            (target.write_mask & static_cast<std::uint8_t>(types::ColorWriteMask::ColorWriteG)) !=
                EMPTY_COLOR_WRITE_MASK,
            (target.write_mask & static_cast<std::uint8_t>(types::ColorWriteMask::ColorWriteB)) !=
                EMPTY_COLOR_WRITE_MASK,
            (target.write_mask & static_cast<std::uint8_t>(types::ColorWriteMask::ColorWriteA)) !=
                EMPTY_COLOR_WRITE_MASK);
    }
} // namespace enishi::renderer::opengl
