#include "updater_pool.h"

namespace enishi::renderer {
    foundation::Option<UpdaterPool::ResourceUpdater&> UpdaterPool::get_updater(
        const types::HandleId handle) {
        return this->handle_mapper.get(handle).and_then(
            [this](const decltype(handle_mapper)::ValueType& mapped_handle) {
                return this->updater.get(mapped_handle.index);
            });
    }
} // namespace enishi::renderer