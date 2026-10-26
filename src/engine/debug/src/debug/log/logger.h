#pragma once
#include "log_entry.h"
#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <fstream>
#include <mutex>
#include <queue>
#include <thread>

namespace enishi::debug {
    class Logger {
      private:
        std::ofstream file_stream;
        std::queue<LogEntry> queue;
        std::mutex queue_mutex;
        std::mutex output_mutex;
        std::thread worker;
        std::condition_variable condition;
        std::atomic<bool> is_running;

        static Logger instance;

      private:
        explicit Logger(void);
        ~Logger(void) noexcept;
        explicit Logger(const Logger&) = delete;
        Logger& operator=(const Logger&) = delete;

        void logging(void);
        void shutdown(void);

        void enqueue(const LogLevel& level, foundation::UTF8&& msg);
        void write(const LogEntry& log);

      public:
        static void debug(foundation::UTF8&& msg);
        static void info(foundation::UTF8&& msg);
        static void warning(foundation::UTF8&& msg);
        static void error(foundation::UTF8&& msg);
    };
} // namespace enishi::debug