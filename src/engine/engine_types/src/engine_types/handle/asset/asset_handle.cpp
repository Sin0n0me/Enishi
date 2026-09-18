#include "asset_handle.h"

namespace enishi::types {
    bool AssetHandle::is_valid(void) const noexcept {
        return this->id.is_valid() && this->type != types::AssetKind::Unknown;
    }
} // namespace enishi::types