#include "physics_handle.h"
#include <cstdint>

namespace enishi::types {
    PhysicsHandle::PhysicsHandle(const HandleId id, const PhysicsHandleType type)
        : id(id)
        , type(type) {
    }

    PhysicsHandle::PhysicsHandle(void)
        : id()
        , type(types::PhysicsHandleType::Unknown) {
    }

    PhysicsHandle::PhysicsHandle(const PhysicsHandle& handle)
        : id(handle.id)
        , type(handle.type) {
    }

    bool PhysicsHandle::is_valid(void) const noexcept {
        return this->id.is_valid() && this->type != types::PhysicsHandleType::Unknown;
    }
} // namespace enishi::types