#include "registry.h"
namespace enishi::ecs {
    EntityID Registry::create(void) {
        return this->handle_allocator.create();
    }

    void Registry::destroy(const EntityID id) {
        // 全Poolから該当エンティティのコンポーネントを削除
        for (auto& [type, pool] : this->pools) {
            if (pool->has(id)) {
                pool->remove(id);
            }
        }
        this->handle_allocator.destroy(id);
    }

    bool Registry::is_alive(const EntityID id) const noexcept {
        return this->handle_allocator.is_alive(id);
    }
} // namespace enishi::ecs