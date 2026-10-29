#pragma once
#include <array>
namespace enishi::platform_impl::native_process {
    constexpr int SEND_FLAGS = 0;
    int create_pipe(std::array<int, 2>& descriptors, bool socket_input);
    bool configure_descriptor(int descriptor, bool socket);
    long descriptor_limit();
    void prepare_exec(long limit);
} // namespace enishi::platform_impl::native_process
