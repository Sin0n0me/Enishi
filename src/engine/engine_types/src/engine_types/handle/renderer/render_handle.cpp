#include "render_handle.h"
#include <cstdint>

namespace enishi::types {
    RenderHandle::RenderHandle(const HandleId id, const RenderHandleType type)
        : id(id)
        , type(type) {
    }

    RenderHandle::RenderHandle(void)
        : id()
        , type(types::RenderHandleType::Unknown) {
    }

    RenderHandle::RenderHandle(const RenderHandle& handle)
        : id(handle.id)
        , type(handle.type) {
    }

    bool RenderHandle::is_valid(void) const noexcept {
        return this->id.is_valid() && this->type != types::RenderHandleType::Unknown;
    }
} // namespace enishi::types