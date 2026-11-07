#include "linux_process_io.h"
#include <fcntl.h>
#include <pty.h>
#include <unistd.h>
namespace enishi::platform_impl::native_process {
    int open_terminal(int& master, int& slave, winsize& size) {
        return openpty(&master, &slave, nullptr, nullptr, &size);
    }
    int create_pipe(std::array<int, 2>& descriptors, bool socket_input) {
        if (socket_input) {
            return socketpair(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0, descriptors.data());
        }
        return pipe2(descriptors.data(), O_CLOEXEC);
    }
} // namespace enishi::platform_impl::native_process
