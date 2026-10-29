#pragma once
#include <expected>
#include <string>
namespace enishi::platform_impl {
    [[nodiscard]] std::expected<void, std::string> launch_shell_terminal();
}
