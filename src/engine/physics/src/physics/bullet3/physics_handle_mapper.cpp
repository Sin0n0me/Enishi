#include "physics_handle_mapper.h"

namespace enishi::physics::bullet3 {
    types::PhysicsHandle PhysicsHandleMapper::make(
        const types::PhysicsHandleType& handle_type, types::ResourceHandles&& mapped) noexcept {
        const auto handle = types::PhysicsHandle{
            this->handle_allocator->create(),
            handle_type,
        };
        this->handle_mapper.emplace(handle, std::move(mapped));

        return handle;
    }

    foundation::Option<types::ResourceHandles&> PhysicsHandleMapper::get(
        const types::PhysicsHandle& handle) noexcept {
        const auto iter = this->handle_mapper.find(handle);
        if (iter == this->handle_mapper.end()) {
            return {};
        }
        return iter->second;
    }

    types::ResourceHandles& PhysicsHandleMapper::operator[](const types::PhysicsHandle& handle) {
        return this->handle_mapper[handle];
    }

    foundation::Option<const types::ResourceHandles&> PhysicsHandleMapper::get(
        const types::PhysicsHandle& handle) const noexcept {
        const auto iter = this->handle_mapper.find(handle);
        if (iter == this->handle_mapper.end()) {
            return {};
        }
        return iter->second;
    }
} // namespace enishi::physics::bullet3