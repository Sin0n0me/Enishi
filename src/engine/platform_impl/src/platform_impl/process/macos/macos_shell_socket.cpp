#include "macos_shell_socket.h"
#include "macos_process_io.h"
#include <cerrno>
#include <fcntl.h>
#include <sys/time.h>
#include <unistd.h>
namespace enishi::platform_impl::native_socket {
    namespace {
        Socket configure_owned_socket(Socket socket) {
            if (socket == INVALID_SOCKET_VALUE ||
                native_process::configure_descriptor(socket, true)) {
                return socket;
            }
            const auto error = errno;
            ::close(socket);
            errno = error;
            return INVALID_SOCKET_VALUE;
        }
    } // namespace
    Socket create_socket() {
        return configure_owned_socket(::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP));
    }
    int socket_error() {
        return errno;
    }
    bool would_block(int error) {
        return error == EAGAIN || error == EWOULDBLOCK;
    }
    void close_socket(Socket value) {
        ::close(value);
    }
    bool nonblocking(Socket value) {
        const auto flags = fcntl(value, F_GETFL);
        return flags != -1 && fcntl(value, F_SETFL, flags | O_NONBLOCK) != -1;
    }
    bool initialize() {
        return true;
    }
    void cleanup() {
    }
    bool set_send_timeout(Socket socket, int milliseconds) {
        constexpr int MILLISECONDS_PER_SECOND = 1000;
        constexpr int MICROSECONDS_PER_MILLISECOND = 1000;
        // seconds = ms / 1000; remaining microseconds = (ms % 1000) * 1000.
        const timeval timeout{milliseconds / MILLISECONDS_PER_SECOND,
            (milliseconds % MILLISECONDS_PER_SECOND) * MICROSECONDS_PER_MILLISECOND};
        return setsockopt(socket, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout)) == 0;
    }
    bool configure_listener(Socket socket) {
        const int reuse = 1;
        return setsockopt(socket, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) == 0;
    }
    Socket accept_socket(Socket socket) {
        return configure_owned_socket(::accept(socket, nullptr, nullptr));
    }
} // namespace enishi::platform_impl::native_socket
