#pragma once
#include "shell_connection.h"
#include <deque>
#include <functional>

namespace enishi::bypass_shell {
    class ShellMonitor {
      public:
        using Receiver = std::function<void(const ShellEvent&)>;

      private:
        struct Session {
            std::uint64_t id;
            std::unique_ptr<ShellConnection> connection;
        };
        ShellConnection listener;
        std::vector<Session> sessions;
        std::vector<Receiver> receivers;
        std::deque<ShellEvent> history;
        std::uint64_t next_session = 1;
        std::size_t history_bytes{};
        std::string error;
        bool listening = false;

      public:
        ShellMonitor();
        ShellResult<void> start(std::uint16_t port = DEFAULT_PORT);
        // Call from the application thread; receivers execute on that same thread.
        void poll();
        void add_receiver(Receiver receiver);
        const std::deque<ShellEvent>& events() const;
        const std::string& last_error() const;
        void clear_history();
        [[nodiscard]] bool is_listening() const;

      private:
        void publish(ShellEvent event);
    };
} // namespace enishi::bypass_shell
