#pragma once
#include <atomic>
#include <csignal>
#include <cstdio>
#include <fcntl.h>
#include <io.h>
#include <string_view>
#include <windows.h>

namespace enishi::platform_impl::native_terminal {
    inline std::atomic<bool> interrupted{false};
    static_assert(std::atomic<bool>::is_always_lock_free);
    inline BOOL WINAPI interrupt(DWORD) {
        interrupted.store(true);
        return TRUE;
    }
    inline void initialize_shell_terminal() {
        SetConsoleCtrlHandler(interrupt, TRUE);
        _setmode(_fileno(stdin), _O_BINARY);
        _setmode(_fileno(stdout), _O_BINARY);
        _setmode(_fileno(stderr), _O_BINARY);
    }
    inline bool shell_interrupted() {
        return interrupted.load();
    }
    inline std::string_view shell_program_name() {
        return "cmd.exe";
    }
} // namespace enishi::platform_impl::native_terminal
