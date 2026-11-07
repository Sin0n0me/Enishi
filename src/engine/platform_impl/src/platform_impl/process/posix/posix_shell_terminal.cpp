#include "posix_shell_terminal.h"
#include <atomic>
#include <csignal>
#include <cstdio>
#include <string_view>

namespace enishi::platform_impl::native_terminal {
    std::atomic<bool> interrupted{false};
    static_assert(std::atomic<bool>::is_always_lock_free);
    void interrupt(int) {
        interrupted.store(true);
    }
    void initialize_shell_terminal() {
        std::signal(SIGINT, interrupt);
        std::signal(SIGTERM, interrupt);
        std::signal(SIGPIPE, SIG_IGN);
    }
    bool shell_interrupted() {
        return interrupted.load();
    }
    std::string_view shell_program_name() {
        return "/bin/sh";
    }
} // namespace enishi::platform_impl::native_terminal
