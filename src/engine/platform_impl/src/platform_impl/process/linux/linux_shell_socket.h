#pragma once
#include <cerrno>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

namespace enishi::platform_impl::native_socket {
    using Socket = int;
    constexpr Socket INVALID_SOCKET_VALUE = -1;
    constexpr int STREAM_SOCKET = SOCK_STREAM | SOCK_CLOEXEC;
    inline int socket_error() {
        return errno;
    }
    inline bool would_block(int error) {
        return error == EAGAIN || error == EWOULDBLOCK;
    }
    inline void close_socket(Socket value) {
        ::close(value);
    }
    inline bool nonblocking(Socket value) {
        const auto flags = fcntl(value, F_GETFL);
        return flags != -1 && fcntl(value, F_SETFL, flags | O_NONBLOCK) != -1;
    }
    constexpr int SEND_FLAGS = MSG_NOSIGNAL;
    inline bool initialize() {
        return true;
    }
    inline void cleanup() {
    }
    inline bool set_send_timeout(Socket socket, int milliseconds) {
        constexpr int MILLISECONDS_PER_SECOND = 1000;
        constexpr int MICROSECONDS_PER_MILLISECOND = 1000;
        // seconds = ms / 1000; remaining microseconds = (ms % 1000) * 1000.
        const timeval timeout{milliseconds / MILLISECONDS_PER_SECOND,
            (milliseconds % MILLISECONDS_PER_SECOND) * MICROSECONDS_PER_MILLISECOND};
        return setsockopt(socket, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout)) == 0;
    }
    inline bool configure_listener(Socket socket) {
        const int reuse = 1;
        return setsockopt(socket, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) == 0;
    }
    inline Socket accept_socket(Socket socket) {
        return ::accept4(socket, nullptr, nullptr, SOCK_CLOEXEC);
    }
} // namespace enishi::platform_impl::native_socket
