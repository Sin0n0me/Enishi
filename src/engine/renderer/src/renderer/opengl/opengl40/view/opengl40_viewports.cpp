#include "opengl40_viewports.h"
#include <glad/gl.h>
#include <renderer/opengl/opengl40/runtime/opengl40_handles.h>

namespace enishi::renderer::opengl {
    platform::RenderResult<types::RenderHandle> OpenGL40Viewports::create_viewport(
        OpenGL40RendererState& runtime,
        RenderHandleMapper& handle_mapper,
        const types::ViewportRect& viewport) {
        if (!runtime.viewport_order.empty()) {
            return foundation::Error(platform::RenderError::MakeError,
                "OpenGL 4.0 supports one viewport without viewport-array extensions");
        }
        const auto handle =
            OpenGL40Handles::make_handle(handle_mapper, types::RenderHandleType::ViewPort);
        runtime.viewports.emplace(handle, viewport);
        runtime.viewport_order.emplace_back(handle);
        return handle;
    }
    void OpenGL40Viewports::setup_viewports(OpenGL40RendererState& runtime) {
        if (runtime.viewport_order.empty()) {
            return;
        }
        const auto viewport = runtime.viewports.find(runtime.viewport_order.front());
        if (viewport == runtime.viewports.end()) {
            return;
        }
        glViewport(static_cast<GLint>(viewport->second.left_top_x),
            static_cast<GLint>(viewport->second.left_top_y),
            static_cast<GLsizei>(viewport->second.width),
            static_cast<GLsizei>(viewport->second.height));
        glDepthRange(viewport->second.min_depth, viewport->second.max_depth);
    }
    void OpenGL40Viewports::submit_command_viewport(
        OpenGL40RendererState& runtime, const types::DrawCommand& command) {
        if (command.sub_command != types::SubCommand::Bind) {
            return;
        }
        const auto viewport = runtime.viewports.find(command.handle);
        if (viewport == runtime.viewports.end()) {
            return;
        }
        glViewport(static_cast<GLint>(viewport->second.left_top_x),
            static_cast<GLint>(viewport->second.left_top_y),
            static_cast<GLsizei>(viewport->second.width),
            static_cast<GLsizei>(viewport->second.height));
        glDepthRange(viewport->second.min_depth, viewport->second.max_depth);
    }
} // namespace enishi::renderer::opengl
