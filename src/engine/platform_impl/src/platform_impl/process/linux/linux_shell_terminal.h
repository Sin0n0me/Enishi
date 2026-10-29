#pragma once
#include <atomic>
#include <csignal>
#include <cstdio>
#include <string_view>

namespace enishi::platform_impl::native_terminal {
    inline std::atomic<bool> interrupted{false};
    static_assert(std::atomic<bool>::is_always_lock_free);
    inline void interrupt(int) {
        interrupted.store(true);
    }
    inline void initialize_shell_terminal() {
        std::signal(SIGINT, interrupt);
        std::signal(SIGTERM, interrupt);
        std::signal(SIGPIPE, SIG_IGN);
    }
    inline bool shell_interrupted() {
        return interrupted.load();
    }
    inline std::string_view shell_program_name() {
        return "/bin/sh";
    }
} // namespace enishi::platform_impl::native_terminal
