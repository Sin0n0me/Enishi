#pragma once
#include <expected>
#include <filesystem>
#include <string>
namespace enishi::platform_impl::native_shell_launcher {
    constexpr const char* EXECUTABLE_NAME = "enishi-shell";
    [[nodiscard]] std::expected<void, std::string> launch(const std::filesystem::path& executable);
} // namespace enishi::platform_impl::native_shell_launcher
