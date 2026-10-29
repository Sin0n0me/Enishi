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
    Socket create_socket();
    int socket_error();
    bool would_block(int error);
    void close_socket(Socket value);
    bool nonblocking(Socket value);
    constexpr int SEND_FLAGS = MSG_NOSIGNAL;
    bool initialize();
    void cleanup();
    bool set_send_timeout(Socket socket, int milliseconds);
    bool configure_listener(Socket socket);
    Socket accept_socket(Socket socket);
} // namespace enishi::platform_impl::native_socket
