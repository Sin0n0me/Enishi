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
        while (this->is_running) {
            std::unique_lock<std::mutex> lock(this->tasks_mutex);
            this->condition.wait(
                lock, [this] { return !this->is_running || !this->tasks.empty(); });

            while (!this->tasks.empty()) {
                const auto&& task = std::move(this->tasks.front());
                this->tasks.pop();
                lock.unlock();

                // タスク内で例外が送出されてもワーカースレッド自体は落とさない
                try {
                    task();
                } catch (...) {
                }

                lock.lock();
            }
        }
    }

    void SingleThreadExecutor::shutdown(void) noexcept {
        this->is_running = false;

        this->condition.notify_all();
        if (this->worker.joinable()) {
            this->worker.join();
        }
    }
} // namespace enishi::foundation