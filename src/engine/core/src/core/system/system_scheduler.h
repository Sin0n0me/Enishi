#pragma once
#include "interface_system.h"
#include <algorithm>
#include <cstdint>
#include <memory>
#include <utility>
#include <vector>

namespace enishi::core {
    using Priority = std::uint32_t;

    class SystemScheduler {
      private:
        struct Entry {
            Priority priority;
            std::shared_ptr<ISystem> system;
        };

      private:
        std::vector<Entry> managed_systems;

      public:
        template <typename T, typename... Args>
        std::shared_ptr<T> register_system(const Priority priority, const Args&... args) {
            auto system = std::make_shared<T>(args...);
            this->managed_systems.emplace_back(Entry{
                .priority = priority,
                .system = system,
            });

            // 初期化時にしか呼ばれないので毎回ソートで構わない
            std::ranges::sort(this->managed_systems,
                [](const Entry& lhs, const Entry& rhs) { return lhs.priority < rhs.priority; });

            return system;
        }

        bool should_close(void);

        void pre_update(void);

        void update(const types::DeltaTime& dt);

        void post_update(void);

        void render(void) const;
    };
} // namespace enishi::core