#include "macos_process_io.h"
#include <fcntl.h>
#include <sys/socket.h>
#include <unistd.h>
#include <util.h>
namespace enishi::platform_impl::native_process {
    int open_terminal(int& master, int& slave, winsize& size) {
        return openpty(&master, &slave, nullptr, nullptr, &size);
    }
    int create_pipe(std::array<int, 2>& descriptors, bool socket_input) {
        if (socket_input) {
            return socketpair(AF_UNIX, SOCK_STREAM, 0, descriptors.data());
        }
        return pipe(descriptors.data());
    }
    bool configure_descriptor(int descriptor, bool socket) {
        const auto flags = fcntl(descriptor, F_GETFD);
        if (flags == -1 || fcntl(descriptor, F_SETFD, flags | FD_CLOEXEC) == -1) {
            return false;
        }
        if (socket) {
            const int enabled = 1;
            return setsockopt(descriptor, SOL_SOCKET, SO_NOSIGPIPE, &enabled, sizeof(enabled)) == 0;
        }
        return true;
    }
    long descriptor_limit() {
        return sysconf(_SC_OPEN_MAX);
    }
    void prepare_exec(long limit) {
        // macOS has no atomic CLOEXEC pipe/socket creation. Close inherited descriptors
        // after dup2, including ones another thread was still configuring when fork ran.
        constexpr int FIRST_NON_STANDARD_DESCRIPTOR = STDERR_FILENO + 1;
        for (long descriptor = FIRST_NON_STANDARD_DESCRIPTOR; descriptor < limit; ++descriptor) {
            close(static_cast<int>(descriptor));
        }
    }
} // namespace enishi::platform_impl::native_process
