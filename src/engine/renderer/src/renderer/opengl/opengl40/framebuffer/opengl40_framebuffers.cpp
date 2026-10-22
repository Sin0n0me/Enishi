#include "opengl40_framebuffers.h"
#include <foundation/log/logger.h>
#include <glad/gl.h>
#include <renderer/opengl/opengl40/support/opengl40_errors.h>
#include <renderer/opengl/opengl40/support/opengl40_helpers.h>

namespace enishi::renderer::opengl {
    namespace {
        constexpr std::uint32_t NO_GL_OBJECT = 0;
        constexpr std::uint32_t SHADOW_TEXTURE_UNIT = 3;
        constexpr std::uint32_t RESOURCE_COUNT = 1;
        constexpr std::int32_t TEXTURE_IMAGE_BORDER_WIDTH = 0;
        constexpr std::uint32_t DEFAULT_FRAMEBUFFER = 0;
        void clear_framebuffer(GLbitfield mask) {
            // glClear obeys the depth write mask left by the preceding pass.
            GLboolean write_mask = GL_FALSE;
            glGetBooleanv(GL_DEPTH_WRITEMASK, &write_mask);
            glDepthMask(GL_TRUE);
            glClear(mask);
            glDepthMask(write_mask);
        }

    } // namespace
    void OpenGL40Framebuffers::setup_views(OpenGL40RendererState& runtime) {
        runtime.back_buffer_cleared = false;
        // Detach the previous frame's sampled depth before rendering into it again.
        glActiveTexture(GL_TEXTURE0 + SHADOW_TEXTURE_UNIT);
        glBindTexture(GL_TEXTURE_2D, NO_GL_OBJECT);
        glBindSampler(SHADOW_TEXTURE_UNIT, NO_GL_OBJECT);
        glActiveTexture(GL_TEXTURE0);
        glBindFramebuffer(GL_FRAMEBUFFER, DEFAULT_FRAMEBUFFER);
        runtime.active_framebuffer_complete = true;
    }
    void OpenGL40Framebuffers::submit_command_view(OpenGL40RendererState& runtime,
        ResourceAccessor& resource_accessor,
        const types::DrawCommand& command,
        const types::RenderHandle& render_target) {
        if (command.sub_command != types::SubCommand::Bind) {
            return;
        }
        const auto view_type = resource_accessor.get_view_type(command.handle.id);
        if (view_type.is_none()) {
            return;
        }
        const auto type = view_type.unwrap();
        if (type == types::ImageViewType::DepthStencil && !render_target.is_valid()) {
            OpenGL40Framebuffers::bind_depth_only_view(runtime, resource_accessor, command.handle);
            return;
        }
        if (type == types::ImageViewType::ShaderResource) {
            const auto object = runtime.objects.find(command.handle);

            if (object != runtime.objects.end()) {
                const auto unit = runtime.view_texture_units.find(command.handle);
                if (unit == runtime.view_texture_units.end()) {
                    return;
                }
                glActiveTexture(GL_TEXTURE0 + unit->second);
                glBindTexture(GL_TEXTURE_2D, object->second);
                glBindSampler(unit->second, NO_GL_OBJECT);
                glActiveTexture(GL_TEXTURE0);
            }
            return;
        }
        const auto object = runtime.objects.find(command.handle);
        if (object == runtime.objects.end()) {
            return;
        }
        const auto is_back_buffer =
            runtime.back_buffer_images.contains(command.handle) ||
            (render_target.is_valid() && runtime.back_buffer_images.contains(render_target));
        if (is_back_buffer) {
            if (type == types::ImageViewType::RenderTarget) {
                if (!OpenGL40Framebuffers::prepare_back_buffer_framebuffer(
                        runtime, command.handle)) {
                    return;
                }
                const auto render_target_view =
                    resource_accessor.get_render_target_view(command.handle.id);
                if (render_target_view.is_none()) {
                    return;
                }
                const auto color = render_target_view.unwrap()->get_clear_color();
                glClearColor(color.r, color.g, color.b, color.a);
                return;
            }
            if (type == types::ImageViewType::DepthStencil) {
                if (!render_target.is_valid() ||
                    !OpenGL40Framebuffers::prepare_back_buffer_framebuffer(
                        runtime, render_target)) {
                    return;
                }
                const auto depth_stencil_view =
                    resource_accessor.get_depth_stencil_view(command.handle.id);
                if (depth_stencil_view.is_none()) {
                    return;
                }
                glClearDepth(depth_stencil_view.unwrap()->clear_depth());
                glClearStencil(depth_stencil_view.unwrap()->clear_stencil());
                const auto depth_stencil_image = runtime.images.find(command.handle);
                if (depth_stencil_image == runtime.images.end()) {
                    return;
                }
                glFramebufferTexture2D(GL_FRAMEBUFFER,
                    helpers::to_gl_depth_attachment(depth_stencil_image->second.format),
                    GL_TEXTURE_2D,
                    object->second,
                    static_cast<GLint>(
                        depth_stencil_view.unwrap()->get_description().base_mip_level));
                if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
                    return;
                }
                runtime.is_back_buffer_framebuffer_complete = true;
                GLbitfield clear_mask = GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT;
                if (!runtime.back_buffer_cleared) {
                    clear_mask |= GL_COLOR_BUFFER_BIT;
                    runtime.back_buffer_cleared = true;
                }
                clear_framebuffer(clear_mask);
            }
            return;
        }
        if (!runtime.active_framebuffer) {
            glGenFramebuffers(RESOURCE_COUNT, &runtime.active_framebuffer);

            runtime.framebuffers.emplace_back(runtime.active_framebuffer);
        }
        glBindFramebuffer(GL_FRAMEBUFFER, runtime.active_framebuffer);
        if (type == types::ImageViewType::RenderTarget) {
            const auto render_target_view =
                resource_accessor.get_render_target_view(command.handle.id);
            if (render_target_view.is_none()) {
                return;
            }
            const auto color = render_target_view.unwrap()->get_clear_color();
            glClearColor(color.r, color.g, color.b, color.a);
            glFramebufferTexture2D(GL_FRAMEBUFFER,
                GL_COLOR_ATTACHMENT0,
                GL_TEXTURE_2D,
                object->second,
                static_cast<GLint>(render_target_view.unwrap()->get_description().base_mip_level));
        }
        if (type == types::ImageViewType::DepthStencil) {
            const auto depth_stencil_view =
                resource_accessor.get_depth_stencil_view(command.handle.id);
            if (depth_stencil_view.is_none()) {
                return;
            }
            glClearDepth(depth_stencil_view.unwrap()->clear_depth());
            glClearStencil(depth_stencil_view.unwrap()->clear_stencil());
            const auto depth_stencil_image = runtime.images.find(command.handle);
            if (depth_stencil_image == runtime.images.end()) {
                return;
            }
            glFramebufferTexture2D(GL_FRAMEBUFFER,
                helpers::to_gl_depth_attachment(depth_stencil_image->second.format),
                GL_TEXTURE_2D,
                object->second,
                static_cast<GLint>(depth_stencil_view.unwrap()->get_description().base_mip_level));
        }
        if (render_target.is_valid() && type == types::ImageViewType::DepthStencil) {
            const auto target = runtime.objects.find(render_target);
            const auto render_target_view =
                resource_accessor.get_render_target_view(render_target.id);

            if (target != runtime.objects.end() && render_target_view.is_some()) {
                const auto color = render_target_view.unwrap()->get_clear_color();
                glClearColor(color.r, color.g, color.b, color.a);
                glFramebufferTexture2D(GL_FRAMEBUFFER,
                    GL_COLOR_ATTACHMENT0,
                    GL_TEXTURE_2D,
                    target->second,
                    static_cast<GLint>(
                        render_target_view.unwrap()->get_description().base_mip_level));
            }
        }
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
            return;
        }
        clear_framebuffer(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    }
    void OpenGL40Framebuffers::present(OpenGL40RendererState& runtime) {
        if (runtime.is_back_buffer_framebuffer_complete) {
            glBindFramebuffer(GL_READ_FRAMEBUFFER, runtime.back_buffer_framebuffer);
            glBindFramebuffer(GL_DRAW_FRAMEBUFFER, DEFAULT_FRAMEBUFFER);
            glBlitFramebuffer(0,
                0,
                runtime.back_buffer_width,
                runtime.back_buffer_height,
                0,
                0,
                runtime.back_buffer_width,
                runtime.back_buffer_height,
                GL_COLOR_BUFFER_BIT,
                GL_NEAREST);
            glBindFramebuffer(GL_FRAMEBUFFER, DEFAULT_FRAMEBUFFER);
        }
    }
    void OpenGL40Framebuffers::bind_depth_only_view(OpenGL40RendererState& runtime,
        ResourceAccessor& resource_accessor,
        const types::RenderHandle& handle) {
        runtime.active_framebuffer_complete = false;
        const auto image = runtime.images.find(handle);
        const auto object = runtime.objects.find(handle);
        const auto view = resource_accessor.get_depth_stencil_view(handle.id);
        if (image == runtime.images.end() || object == runtime.objects.end() || view.is_none()) {
            foundation::Logger::error("OpenGL depth-only view resources are missing");
            return;
        }
        auto [entry, inserted] = runtime.depth_framebuffers.try_emplace(handle, NO_GL_OBJECT);
        if (inserted) {
            glGenFramebuffers(RESOURCE_COUNT, &entry->second);
            runtime.framebuffers.push_back(entry->second);
        }
        glBindFramebuffer(GL_FRAMEBUFFER, entry->second);
        glFramebufferTexture2D(GL_FRAMEBUFFER,
            helpers::to_gl_depth_attachment(image->second.format),
            GL_TEXTURE_2D,
            object->second,
            static_cast<GLint>(view.unwrap()->get_description().base_mip_level));
        glDrawBuffer(GL_NONE);
        glReadBuffer(GL_NONE);
        const auto status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
        if (status != GL_FRAMEBUFFER_COMPLETE) {
            foundation::Logger::error(
                std::format("OpenGL depth framebuffer incomplete: {:#x}", status));
            return;
        }
        glViewport(0, 0, image->second.size.x, image->second.size.y);
        glDepthRange(0, 1);
        glClearDepth(view.unwrap()->clear_depth());
        clear_framebuffer(GL_DEPTH_BUFFER_BIT);
        const auto result = check_gl_errors("depth-only framebuffer setup");
        if (result.is_err()) {
            foundation::Logger::error(result.unwrap_err().get_message());
            return;
        }
        runtime.active_framebuffer_complete = true;
    }
    bool OpenGL40Framebuffers::prepare_back_buffer_framebuffer(
        OpenGL40RendererState& runtime, const types::RenderHandle& render_target) {
        const auto image = runtime.images.find(render_target);
        if (image == runtime.images.end()) {
            return false;
        }
        if (!runtime.back_buffer_framebuffer) {
            glGenFramebuffers(RESOURCE_COUNT, &runtime.back_buffer_framebuffer);
            runtime.framebuffers.emplace_back(runtime.back_buffer_framebuffer);
        }
        if (!runtime.back_buffer_color) {
            glGenTextures(RESOURCE_COUNT, &runtime.back_buffer_color);
            runtime.textures.emplace_back(runtime.back_buffer_color);
        }
        glBindTexture(GL_TEXTURE_2D, runtime.back_buffer_color);
        // Rebinding the main target for another pass must preserve the background shadow.
        if (runtime.back_buffer_width != image->second.size.x ||
            runtime.back_buffer_height != image->second.size.y) {
            glTexImage2D(GL_TEXTURE_2D,
                TEXTURE_IMAGE_BORDER_WIDTH,
                helpers::to_gl_image_internal_format(image->second.format),
                image->second.size.x,
                image->second.size.y,
                TEXTURE_IMAGE_BORDER_WIDTH,
                helpers::to_gl_image_format(image->second.format),
                helpers::to_gl_image_type(image->second.format),
                nullptr);
        }
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glBindFramebuffer(GL_FRAMEBUFFER, runtime.back_buffer_framebuffer);
        glViewport(0, 0, image->second.size.x, image->second.size.y);
        glDepthRange(0, 1);
        runtime.active_framebuffer_complete = true;
        glFramebufferTexture2D(GL_FRAMEBUFFER,
            GL_COLOR_ATTACHMENT0,
            GL_TEXTURE_2D,
            runtime.back_buffer_color,
            TEXTURE_IMAGE_BORDER_WIDTH);
        runtime.back_buffer_width = image->second.size.x;
        runtime.back_buffer_height = image->second.size.y;
        runtime.is_back_buffer_framebuffer_complete = false;
        return true;
    }
} // namespace enishi::renderer::opengl
