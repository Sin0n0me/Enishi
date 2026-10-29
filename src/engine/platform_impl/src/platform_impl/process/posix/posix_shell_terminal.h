#pragma once
#include <string_view>
namespace enishi::platform_impl::native_terminal {
    void initialize_shell_terminal();
    bool shell_interrupted();
    std::string_view shell_program_name();
} // namespace enishi::platform_impl::native_terminal
