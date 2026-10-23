#include "opengl40_render_command_encoder.h"
#include <renderer/opengl/opengl40/command/opengl40_commands.h>
#include <renderer/opengl/opengl40/framebuffer/opengl40_framebuffers.h>
#include <renderer/opengl/opengl40/pipeline/opengl40_render_states.h>
#include <renderer/opengl/opengl40/shader/opengl40_shaders.h>
#include <renderer/opengl/opengl40/view/opengl40_viewports.h>
#include <renderer/opengl/opengl40/view/opengl40_views.h>

namespace enishi::renderer::opengl {

    OpenGL40RenderCommandEncoder::OpenGL40RenderCommandEncoder(OpenGL40RendererState& state,
        ResourceAccessor& resource_accessor,
        platform::IOpenGLContext& context)
        : state(state)
        , resource_accessor(resource_accessor)
        , context(context) {
    }
    void OpenGL40RenderCommandEncoder::submit_command_buffer(const types::DrawCommand&) const {
    }
    void OpenGL40RenderCommandEncoder::submit_command_vertex_layout(
        const types::DrawCommand&) const {
    }
    void OpenGL40RenderCommandEncoder::setup_viewports(void) const {
        OpenGL40Viewports::setup_viewports(this->state);
    }
    void OpenGL40RenderCommandEncoder::submit_command_viewport(
        const types::DrawCommand& command) const {
        OpenGL40Viewports::submit_command_viewport(this->state, command);
    }
    void OpenGL40RenderCommandEncoder::submit_command_shader(
        const types::DrawCommand& command) const {
        OpenGL40Shaders::submit_command_shader(this->state, command);
    }
    void OpenGL40RenderCommandEncoder::submit_command_state(
        const types::DrawCommand& command) const {
        OpenGL40RenderStates::submit_command_state(this->state, command);
    }
    void OpenGL40RenderCommandEncoder::setup_views(void) const {
        OpenGL40Framebuffers::setup_views(this->state);
    }
    void OpenGL40RenderCommandEncoder::submit_command_view(
        const types::DrawCommand& command, const types::RenderHandle& render_target) const {
        OpenGL40Framebuffers::submit_command_view(
            this->state, this->resource_accessor, command, render_target);
    }
    void OpenGL40RenderCommandEncoder::present(void) const {
        OpenGL40Framebuffers::present(this->state);
        this->context.present();
    }
    void OpenGL40RenderCommandEncoder::submit_command_mesh(
        const types::DrawCommand& command, const types::RenderHandle& render_target) const {
        OpenGL40Commands::submit_command_mesh(
            this->state, this->resource_accessor, command, render_target);
    }
    void OpenGL40RenderCommandEncoder::submit_command_topology(
        const types::DrawCommand& command) const {
        OpenGL40Commands::submit_command_topology(this->state, command);
    }
    void OpenGL40RenderCommandEncoder::submit_command_image(
        const types::DrawCommand& command) const {
        OpenGL40Commands::submit_command_image(this->state, command);
    }
    void OpenGL40RenderCommandEncoder::draw(const types::RenderHandle& handle) const {
        OpenGL40Commands::draw(this->state, handle);
    }
} // namespace enishi::renderer::opengl
