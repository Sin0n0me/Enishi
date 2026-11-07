#pragma once
#include <array>
#include <sys/ioctl.h>
#include <sys/socket.h>
namespace enishi::platform_impl::native_process {
    constexpr int SEND_FLAGS = MSG_NOSIGNAL;
    int open_terminal(int& master, int& slave, winsize& size);
    int create_pipe(std::array<int, 2>& descriptors, bool socket_input);
    constexpr bool configure_descriptor(int, bool) {
        return true;
    }
    constexpr long descriptor_limit() {
        return 0;
    }
    constexpr void prepare_exec(long) {
    }
} // namespace enishi::platform_impl::native_process
