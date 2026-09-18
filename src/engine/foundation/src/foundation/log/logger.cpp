#include "logger.h"
#include <chrono>
#include <iostream>

namespace enishi::foundation {
    decltype(Logger::instance) Logger::instance = Logger{};

    Logger::Logger(void) {
        this->is_running = true;
        this->file_stream.open("log.txt");
        this->worker = std::thread(&Logger::logging, this);
    }

    Logger::~Logger(void) noexcept {
        this->shutdown();
        this->file_stream.close();
    }

    void Logger::logging(void) {
        while (this->is_running) {
            std::unique_lock<std::mutex> lock(this->queue_mutex);
            this->condition.wait(
                lock, [this] { return !this->is_running || !this->queue.empty(); });

            while (!this->queue.empty()) {
                const LogEntry entry = std::move(this->queue.front());
                this->queue.pop();
                lock.unlock();

                this->write(entry);

                lock.lock();
            }
        }
    }

    void Logger::shutdown(void) {
        this->is_running = false;

        this->condition.notify_all();
        if (this->worker.joinable()) {
            this->worker.join();
        }
    }

    void Logger::enqueue(const LogLevel& level, UTF8&& msg) {
        {
            std::lock_guard<std::mutex> lock(this->queue_mutex);
            this->queue.push(LogEntry{level, msg, std::chrono::system_clock::now()});
        }

        this->condition.notify_one();
    }

    void Logger::write(const LogEntry& log_entry) {
        std::lock_guard<std::mutex> lock(this->output_mutex);

        const auto log = log_entry.format_log();

        if (this->file_stream.is_open()) {
            this->file_stream.write(reinterpret_cast<const char*>(log.data()),
                static_cast<std::streamsize>(log.size()));
            this->file_stream.flush();
        }

        std::cout.write(
            reinterpret_cast<const char*>(log.data()), static_cast<std::streamsize>(log.size()));
    }

    void Logger::debug(UTF8&& msg) {
        Logger::instance.enqueue(LogLevel::Debug, std::move(msg));
    }

    void Logger::info(UTF8&& msg) {
        Logger::instance.enqueue(LogLevel::Info, std::move(msg));
    }

    void Logger::warning(UTF8&& msg) {
        Logger::instance.enqueue(LogLevel::Warning, std::move(msg));
    }

    void Logger::error(UTF8&& msg) {
        Logger::instance.enqueue(LogLevel::Error, std::move(msg));
    }
} // namespace enishi::foundation