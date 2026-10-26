#pragma once
#include <cstddef>
#include <cstdint>
#include <string>

namespace enishi::bypass_shell {
    constexpr std::uint16_t DEFAULT_PORT = 39841;
    constexpr std::size_t MAX_PAYLOAD = 16384;
    enum class ShellEventKind : std::uint8_t {
        Started = 1,
        Input,
        Stdout,
        Stderr,
        Exited,
        Disconnected
    };
    struct ShellEvent {
        std::uint64_t session{};
        ShellEventKind kind{};
        // Original bytes, not necessarily UTF-8. Input is forwarded data, not parsed shell syntax.
        std::string data;
    };
} // namespace enishi::bypass_shell
