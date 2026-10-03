#include "system_handle.h"

namespace enishi::types {
    bool SystemHandle::is_valid(void) const noexcept {
        return this->id.is_valid();
    }
} // namespace enishi::types