#pragma once
#include <atomic>
#include <condition_variable>
#include <cstddef>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>

namespace enishi::foundation {
    /**
     * この型の目的
     *
     * 唯一のワーカースレッドでタスクをFIFOに直列実行するエグゼキュータ
     * 常駐スレッドは常に1本のみで、submitされたタスクは積まれた順に実行される
     *
     * IO待ちなど時間のかかる処理をメインスレッドから切り離しつつ、
     * 使用可能なIOスレッドが1本しかない状況で複数のIO要求を安全に捌くために使用する
     * (=同時に2つ以上のタスクが並行実行されることはない)
     */
    class SingleThreadExecutor {
      private:
        std::queue<std::function<void(void)>> tasks;
        mutable std::mutex tasks_mutex;
        std::condition_variable condition;
        std::thread worker;
        std::atomic<bool> is_running;

      public:
        explicit SingleThreadExecutor(void);
        ~SingleThreadExecutor(void) noexcept;

        SingleThreadExecutor(const SingleThreadExecutor&) = delete;
        SingleThreadExecutor& operator=(const SingleThreadExecutor&) = delete;
        SingleThreadExecutor(SingleThreadExecutor&&) = delete;
        SingleThreadExecutor& operator=(SingleThreadExecutor&&) = delete;

        // タスクをキューへ積む
        // 呼び出し元のスレッドをブロックしない
        void submit(std::function<void(void)>&& task);

        // 現在キューに積まれている(実行待ちの)タスク数
        // 主にデバッグ, テスト用
        [[nodiscard]] std::size_t pending_task_count(void) const;

      private:
        void worker_loop(void);
        void shutdown(void) noexcept;
    };
} // namespace enishi::foundation
