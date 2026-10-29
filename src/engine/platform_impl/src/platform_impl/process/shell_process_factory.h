#pragma once
#include <memory>
#include <platform/process/interface_shell_process.h>

namespace enishi::platform_impl {
    [[nodiscard]] std::unique_ptr<platform::IShellProcess> create_shell_process();
}
