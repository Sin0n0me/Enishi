#pragma once
#include <platform/renderer/interface_renderer.h>
namespace enishi::renderer::opengl {
    platform::RenderResult<void> check_gl_errors(const char* operation);
} // namespace enishi::renderer::opengl
