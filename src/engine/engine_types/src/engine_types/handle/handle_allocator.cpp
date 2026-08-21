#include "handle_allocator.h"

namespace enishi::types {
    HandleAllocator::HandleAllocator(void)
        : next_id(0) {
    }

    HandleId HandleAllocator::create(void) {
        if (!this->free_list.empty()) {
            // 削除済みIDを再利用
            const HandleId handle = this->free_list.back();
            const auto index = handle.handle_id;
            this->free_list.pop_back();
            this->alive[index] = true;
            return handle;
        }

        const HandleId handle = this->next_id;
        this->next_id.handle_id += 1;
        this->alive.push_back(true);

        return handle;
    }

    void HandleAllocator::destroy(const HandleId handle) {
        const auto index = handle.handle_id;
        if (index < this->alive.size()) {
            this->alive[index] = false;
            this->free_list.push_back(handle);
        }
    }

    bool HandleAllocator::is_alive(const HandleId handle) const noexcept {
        const auto index = handle.handle_id;
        return index < this->alive.size() && this->alive[index];
    }
} // namespace enishi::types