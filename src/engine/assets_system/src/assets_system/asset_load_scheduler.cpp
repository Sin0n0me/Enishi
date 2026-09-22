#include "asset_load_scheduler.h"

namespace enishi::assets_system {
    void AssetLoadScheduler::submit(
        const types::AssetHandle& handle, const std::filesystem::path& path, LoadJob job) {
        // candidatesやpathなど読み込みに必要な情報はjob側にすでに閉じ込められている想定のため、
        // ここではjobの実行と、その結果の受け渡しのみを行う
        this->io_executor.submit([this, handle, path, job = std::move(job)] {
            auto result = job();

            {
                const std::lock_guard<std::mutex> lock(this->completed_mutex);
                this->completed_loads.insert_or_assign(handle,
                    CompletedLoad{
                        .handle = handle,
                        .path = path,
                        .result = std::move(result),
                    });
            }

            this->completed_condition.notify_all();
        });
    }

    foundation::Option<AssetLoadScheduler::CompletedLoad> AssetLoadScheduler::try_take_completed(
        void) noexcept {
        const std::lock_guard<std::mutex> lock(this->completed_mutex);
        if (this->completed_loads.empty()) {
            return {};
        }

        auto node = this->completed_loads.extract(this->completed_loads.begin());
        return std::move(node.mapped());
    }

    foundation::Option<AssetLoadScheduler::CompletedLoad>
    AssetLoadScheduler::wait_and_take_completed(const types::AssetHandle& handle) noexcept {
        std::unique_lock<std::mutex> lock(this->completed_mutex);
        this->completed_condition.wait(
            lock, [this, &handle] { return this->completed_loads.contains(handle); });

        auto node = this->completed_loads.extract(handle);
        if (node.empty()) {
            return {};
        }
        return std::move(node.mapped());
    }
} // namespace enishi::assets_system