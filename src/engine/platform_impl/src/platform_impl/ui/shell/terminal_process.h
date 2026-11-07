#pragma once
#include <expected>
#include <string>
namespace enishi::platform_impl {
    [[nodiscard]] std::expected<void, std::string> launch_terminal_process(
        const char* const* arguments);
}
