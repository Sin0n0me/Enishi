#include "main.h"
#include <atomic>
#include <bypass_shell/shell_process.h>
#include <charconv>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <platform_impl/process/shell_process_factory.h>
#include <platform_impl/process/shell_terminal.h>
#include <platform_impl/process/terminal_input.h>
#include <thread>

namespace {
    using enishi::bypass_shell::ShellEventKind;
    constexpr auto POLL_INTERVAL = std::chrono::milliseconds(10);
    constexpr int INTERRUPTED_EXIT = 130;
    class Relay {
      public:
        enishi::bypass_shell::ShellConnection connection;
        enishi::bypass_shell::ShellProcess process{enishi::platform_impl::create_shell_process()};
        std::atomic<bool> done{false};
        std::atomic<bool> failed{false};
        std::atomic<bool> disconnected{false};
        inline void notify(ShellEventKind kind, std::string_view data) {
            if (this->disconnected.load()) {
                return;
            }
            do {
                const auto chunk = data.substr(0, enishi::bypass_shell::MAX_PAYLOAD);
                auto result = this->connection.send(kind, chunk);
                if (!result.has_value()) {
                    if (!this->disconnected.exchange(true)) {
                        std::fprintf(stderr,
                            "Enishi disconnected: %s; shell remains active.\n",
                            result.error().c_str());
                    }
                    return;
                }
                data.remove_prefix(chunk.size());
            } while (!data.empty());
        }
        inline void output(ShellEventKind kind, FILE* destination) {
            for (;;) {
                auto data = this->process.read(kind);
                if (!data.has_value()) {
                    std::fprintf(stderr, "%s\n", data.error().c_str());
                    this->failed.store(true);
                    return;
                }
                if (!data->empty()) {
                    if (std::fwrite(data->data(), 1, data->size(), destination) != data->size() ||
                        std::fflush(destination) != 0) {
                        this->failed.store(true);
                        return;
                    }
                    this->notify(kind, *data);
                } else {
                    if (this->done.load()) {
                        return;
                    }
                    std::this_thread::sleep_for(POLL_INTERVAL);
                }
            }
        }
        inline int input(enishi::platform_impl::TerminalInput& terminal) {
            bool closed = false;
            while (!enishi::platform_impl::shell_interrupted() && !this->failed.load()) {
                auto status = this->process.exit_code();
                if (!status.has_value()) {
                    std::fprintf(stderr, "%s\n", status.error().c_str());
                    return EXIT_FAILURE;
                }
                if (status->has_value()) {
                    return **status;
                }
                if (!closed) {
                    auto bytes = terminal.read();
                    if (!bytes.has_value()) {
                        std::fprintf(stderr, "%s\n", bytes.error().c_str());
                        return EXIT_FAILURE;
                    }
                    if (bytes->has_value()) {
                        if ((**bytes).empty()) {
                            this->process.close_input();
                            closed = true;
                        } else {
                            this->notify(ShellEventKind::Input, **bytes);
                            auto sent = this->process.write(**bytes);
                            if (!sent.has_value()) {
                                std::fprintf(stderr, "%s\n", sent.error().c_str());
                                return EXIT_FAILURE;
                            }
                        }
                    }
                }
                std::this_thread::sleep_for(POLL_INTERVAL);
            }
            return enishi::platform_impl::shell_interrupted() ? INTERRUPTED_EXIT : EXIT_FAILURE;
        }
    };
    int run(std::uint16_t port) {
        Relay relay;
        auto connected = relay.connection.connect(port);
        if (!connected.has_value()) {
            std::fprintf(stderr, "%s\n", connected.error().c_str());
            return EXIT_FAILURE;
        }
        enishi::platform_impl::TerminalInput terminal;
        auto started = relay.process.start();
        if (!started.has_value()) {
            std::fprintf(stderr, "%s\n", started.error().c_str());
            return EXIT_FAILURE;
        }
        relay.notify(ShellEventKind::Started, enishi::platform_impl::shell_program_name());
        std::jthread output([&] { relay.output(ShellEventKind::Stdout, stdout); });
        std::jthread errors([&] { relay.output(ShellEventKind::Stderr, stderr); });
        // Killing the job/group also unblocks a pipe write when the child is not reading stdin.
        std::jthread cancellation([&](std::stop_token stop) {
            while (!stop.stop_requested()) {
                if (enishi::platform_impl::shell_interrupted() || relay.failed.load()) {
                    relay.process.terminate();
                    return;
                }
                std::this_thread::sleep_for(POLL_INTERVAL);
            }
        });
        auto code = relay.input(terminal);
        cancellation.request_stop();
        cancellation.join();
        if (enishi::platform_impl::shell_interrupted()) {
            code = INTERRUPTED_EXIT;
        }
        relay.process.terminate();
        relay.done.store(true);
        output.join();
        errors.join();
        relay.notify(ShellEventKind::Exited, std::to_string(code));
        return code;
    }
} // namespace
int main(int argc, char** argv) {
    std::uint16_t port = enishi::bypass_shell::DEFAULT_PORT;
    if (argc != 1) {
        constexpr int PORT_ARGUMENT_COUNT = 3;
        if (argc != PORT_ARGUMENT_COUNT || std::string_view(argv[1]) != "--port") {
            std::fprintf(stderr,
                "Usage: enishi-shell [--port NUMBER]\nStart Enishi first. Commands run in cmd.exe "
                "(Windows) or /bin/sh (Linux/macOS).\nInteractive sessions use a pseudo terminal; "
                "Ctrl+C is forwarded to the child. Redirected sessions use pipes.\n");
            return std::string_view(argv[1]) == "--help" ? EXIT_SUCCESS : EXIT_FAILURE;
        }
        const std::string_view argument(argv[2]);
        const auto parsed =
            std::from_chars(argument.data(), argument.data() + argument.size(), port);
        if (parsed.ec != std::errc{} || parsed.ptr != argument.data() + argument.size() ||
            port == 0) {
            std::fprintf(stderr, "Invalid port\n");
            return EXIT_FAILURE;
        }
    }
    enishi::platform_impl::initialize_shell_terminal();
    return run(port);
}
