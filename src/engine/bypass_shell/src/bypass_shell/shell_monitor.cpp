#include "shell_monitor.h"
#include <utility>

namespace enishi::bypass_shell {
    namespace {
        constexpr std::size_t MAX_SESSIONS = 8;
        constexpr std::size_t MAX_HISTORY_EVENTS = 256;
        constexpr std::size_t MAX_HISTORY_BYTES = 262144;
    } // namespace
    ShellMonitor::ShellMonitor() = default;
    ShellResult<void> ShellMonitor::start(std::uint16_t port) {
        if (this->listening) {
            return std::unexpected("Shell monitor is already listening");
        }
        auto result = this->listener.listen(port);
        this->listening = result.has_value();
        this->error = result.has_value() ? std::string{} : result.error();
        return result;
    }
    void ShellMonitor::poll() {
        if (!this->listening) {
            return;
        }
        // One accept and one bounded read per session keep rendering work bounded.
        if (this->sessions.size() < MAX_SESSIONS) {
            auto incoming = this->listener.accept();
            if (!incoming.has_value()) {
                this->error = incoming.error();
            } else if (*incoming != nullptr) {
                this->sessions.push_back({this->next_session++, std::move(*incoming)});
            }
        }
        for (auto session = this->sessions.begin(); session != this->sessions.end();) {
            auto batch = session->connection->receive();
            if (!batch.has_value()) {
                this->publish({session->id, ShellEventKind::Disconnected, batch.error()});
                session = this->sessions.erase(session);
                continue;
            }
            for (auto& event : *batch) {
                event.session = session->id;
                this->publish(std::move(event));
            }
            ++session;
        }
    }
    void ShellMonitor::publish(ShellEvent event) {
        // Snapshot permits receivers to register another receiver during dispatch.
        const auto callbacks = this->receivers;
        for (const auto& callback : callbacks) {
            callback(event);
        }
        this->history_bytes += event.data.size();
        this->history.push_back(std::move(event));
        while (
            this->history.size() > MAX_HISTORY_EVENTS || this->history_bytes > MAX_HISTORY_BYTES) {
            this->history_bytes -= this->history.front().data.size();
            this->history.pop_front();
        }
    }
    void ShellMonitor::add_receiver(Receiver receiver) {
        if (static_cast<bool>(receiver)) {
            this->receivers.push_back(std::move(receiver));
        }
    }
    const std::deque<ShellEvent>& ShellMonitor::events() const {
        return this->history;
    }
    const std::string& ShellMonitor::last_error() const {
        return this->error;
    }
    void ShellMonitor::clear_history() {
        this->history.clear();
        this->history_bytes = 0;
    }
} // namespace enishi::bypass_shell
