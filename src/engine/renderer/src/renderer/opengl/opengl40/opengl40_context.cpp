#include "opengl40_context.h"
#include <glad/gl.h>

#if defined(_WIN32)
#include <Windows.h>
#endif

namespace enishi::renderer::opengl {
#if defined(_WIN32)
    namespace {
        void* get_open_gl_proc_address(const char* const name) {
            const auto proc = wglGetProcAddress(name);
            if (proc != nullptr && proc != reinterpret_cast<PROC>(1) &&
                proc != reinterpret_cast<PROC>(2) && proc != reinterpret_cast<PROC>(3) &&
                proc != reinterpret_cast<PROC>(-1)) {
                return reinterpret_cast<void*>(proc);
            }
            const auto module = GetModuleHandleW(L"opengl32.dll");
            return module ? reinterpret_cast<void*>(GetProcAddress(module, name)) : nullptr;
        }
    } // namespace
#endif

    OpenGL40Context::~OpenGL40Context(void) noexcept {
#if defined(_WIN32)
        if (this->context) {
            wglMakeCurrent(nullptr, nullptr);
            wglDeleteContext(this->context);
        }
        if (this->device_context) {
            ReleaseDC(this->window, this->device_context);
        }
#endif
    }

    foundation::Result<std::shared_ptr<OpenGL40Context>, platform::RenderError>
    OpenGL40Context::make(const platform::WindowHandle& window) {
#if defined(_WIN32)
        if (window.tag != platform::WindowSystem::Windows || !window.native_handle.windows.hwnd) {
            return foundation::Error(platform::RenderError::MakeError,
                "OpenGL 4.0 requires a Windows native window handle");
        }

        auto result = std::shared_ptr<OpenGL40Context>(new OpenGL40Context());
        result->window = window.native_handle.windows.hwnd;
        result->device_context = GetDC(result->window);
        if (!result->device_context) {
            return foundation::Error(platform::RenderError::MakeError, "Failed to obtain the window device context");
        }
        result->context = wglCreateContext(result->device_context);
        if (!result->context || !result->make_current()) {
            return foundation::Error(platform::RenderError::MakeError, "Failed to create an OpenGL context");
        }
        if (!gladLoadGL(get_open_gl_proc_address) || !GLAD_GL_VERSION_4_0) {
            return foundation::Error(platform::RenderError::MakeError, "OpenGL 4.0 is not available");
        }
        return result;
#else
        (void)window;
        return foundation::Error(platform::RenderError::MakeError,
            "The OpenGL context implementation for this window system is not available");
#endif
    }

    bool OpenGL40Context::make_current(void) const noexcept {
#if defined(_WIN32)
        return wglMakeCurrent(this->device_context, this->context) == TRUE;
#else
        return false;
#endif
    }

    void OpenGL40Context::present(void) const noexcept {
#if defined(_WIN32)
        SwapBuffers(this->device_context);
#endif
    }
} // namespace enishi::renderer::opengl
