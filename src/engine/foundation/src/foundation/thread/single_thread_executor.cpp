#include "single_thread_executor.h"

namespace enishi::foundation {
    SingleThreadExecutor::SingleThreadExecutor(void) {
        this->is_running = true;
        this->worker = std::thread(&SingleThreadExecutor::worker_loop, this);
    }

    SingleThreadExecutor::~SingleThreadExecutor(void) noexcept {
        this->shutdown();
    }

    void SingleThreadExecutor::submit(std::function<void(void)>&& task) {
        {
            const std::lock_guard<std::mutex> lock(this->tasks_mutex);
            this->tasks.push(std::move(task));
        }

        this->condition.notify_one();
    }

    std::size_t SingleThreadExecutor::pending_task_count(void) const {
        const std::lock_guard<std::mutex> lock(this->tasks_mutex);
        return this->tasks.size();
    }

    void SingleThreadExecutor::worker_loop(void) {
        while (true) {
            std::unique_lock<std::mutex> lock(this->tasks_mutex);
            this->condition.wait(
                lock, [this] { return !this->is_running || !this->tasks.empty(); });

            if (this->tasks.empty() && !this->is_running) {
                return;
            }
            // Queue ownership changes under the same lock used by producers.
            // Execute outside the lock so tasks may submit more work.
            auto task = std::move(this->tasks.front());
            this->tasks.pop();
            lock.unlock();
            try {
                task();
            } catch (...) {
                // A failed task must not prevent subsequent queued work from running.
            }
        }
    }

    void SingleThreadExecutor::shutdown(void) noexcept {
        {
            const std::lock_guard<std::mutex> lock(this->tasks_mutex);
            this->is_running = false;
        }

        this->condition.notify_all();
        if (this->worker.joinable()) {
            this->worker.join();
        }
    }
} // namespace enishi::foundation
