#include "shell_process_factory.h"
#if defined(_WIN32)
#include "windows/windows_shell_process.h"
#elif defined(__linux__) || defined(__APPLE__)
#include "posix/posix_shell_process.h"
#else
#error Unsupported shell process platform
#endif
namespace enishi::platform_impl {
    std::unique_ptr<platform::IShellProcess> create_shell_process() {
        return std::make_unique<NativeShellProcess>();
    }
} // namespace enishi::platform_impl
