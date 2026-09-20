#pragma once

#include <foundation/result/result.h>
#include <memory>
#include <platform/errors/renderer_errors.h>
#include <platform/window/window_handle.h>

namespace enishi::renderer::opengl {
    class OpenGL40Context {
      private:
#if defined(_WIN32)
        HGLRC context = nullptr;
        HDC device_context = nullptr;
        HWND window = nullptr;
#endif

        OpenGL40Context(void) = default;

      public:
        ~OpenGL40Context(void) noexcept;

        OpenGL40Context(const OpenGL40Context&) = delete;
        OpenGL40Context& operator=(const OpenGL40Context&) = delete;

        [[nodiscard]] static foundation::Result<std::shared_ptr<OpenGL40Context>,
            platform::RenderError>
        make(const platform::WindowHandle& window);

        [[nodiscard]] bool make_current(void) const noexcept;
        void present(void) const noexcept;
    };
} // namespace enishi::renderer::opengl
