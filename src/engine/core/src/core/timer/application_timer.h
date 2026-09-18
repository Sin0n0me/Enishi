#pragma once

#include <engine_types/system/delta_time.h>

namespace enishi::core {
    class ApplicationTimer {
      private:
        std::chrono::steady_clock::time_point last_time;
        std::int64_t elapsed;

      public:
        static constexpr float MAX_FPS = 15.0f;
        static constexpr types::DeltaTime MAX_DELTA_TIME =
            types::DeltaTime(1.0f / ApplicationTimer::MAX_FPS);

      public:
        ApplicationTimer(void);

        // タイマーの内部状態のリセット
        void reset(void) noexcept;

        // 前フレームからの経過時間を返す
        [[nodiscard]] types::DeltaTime tick(void) noexcept;

        // 前フレームからの経過時間を返す(制限なし)
        [[nodiscard]] types::DeltaTime tick_unclamp(void) noexcept;

      private:
        // スパイク対策: 最大デルタタイムを制限する
        [[nodiscard]] types::DeltaTime clamp(const types::DeltaTime& dt) const noexcept;
    };
} // namespace enishi::core