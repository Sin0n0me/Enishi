#pragma once

#include <memory>

namespace enishi::platform {
    class IOpenGLContext {
      public:
        virtual ~IOpenGLContext(void) noexcept = default;

        [[nodiscard]] virtual bool make_current(void) const noexcept = 0;
        virtual void present(void) const noexcept = 0;
        [[nodiscard]] virtual void* get_proc_address(const char* name) const noexcept = 0;
    };
} // namespace enishi::platform
