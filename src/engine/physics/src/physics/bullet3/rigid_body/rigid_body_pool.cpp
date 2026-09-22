#include "rigid_body_pool.h"

namespace enishi::physics::bullet3 {
    std::tuple<types::HandleId, RigidBodyPool::NativeRigidBody&>
    RigidBodyPool::emplace_native_rigid_body(NativeRigidBody&& rigid_body) noexcept {
        return this->handle_mapper.make_from(
            this->native_rigid_bodies.emplace(std::move(rigid_body)), [](const std::size_t index) {
                return decltype(handle_mapper)::ValueType{
                    .index = index,
                };
            });
    }

    void RigidBodyPool::remove_native_rigid_body(const types::HandleId handle) noexcept {
    }

    foundation::Option<RigidBodyPool::NativeRigidBody&> RigidBodyPool::get_native_rigid_body(
        const types::HandleId handle) noexcept {
        return this->handle_mapper.get(handle).and_then(
            [this](const decltype(handle_mapper)::ValueType& mapped) {
                return this->native_rigid_bodies.get(mapped.index);
            });
    }

    foundation::Option<const RigidBodyPool::NativeRigidBody&> RigidBodyPool::get_native_rigid_body(
        const types::HandleId handle) const noexcept {
        return this->handle_mapper.get(handle).and_then(
            [this](const decltype(handle_mapper)::ValueType& mapped) {
                return this->native_rigid_bodies.get(mapped.index);
            });
    }

    std::span<const RigidBodyPool::NativeRigidBody> RigidBodyPool::get_native_rigid_bodies(
        void) const noexcept {
        return this->native_rigid_bodies.get_all();
    }

    foundation::Option<RigidBodyPool::RigidBody&> RigidBodyPool::link_rigid_body(
        const types::HandleId handle, RigidBody&& rigid_body) noexcept {
        if (this->handle_mapper.contains(handle)) {
            return {};
        }

        auto [index, rb] = this->rigid_bodies.emplace(std::move(rigid_body));
        this->handle_mapper[handle].config = index;

        return rb;
    }

    foundation::Option<RigidBodyPool::RigidBody&> RigidBodyPool::get_rigid_body(
        const types::HandleId handle) noexcept {
        return this->handle_mapper.get(handle).and_then(
            [this](const decltype(handle_mapper)::ValueType& mapped) {
                return this->rigid_bodies.get(mapped.config);
            });
    }

    foundation::Option<const RigidBodyPool::RigidBody&> RigidBodyPool::get_rigid_body(
        const types::HandleId handle) const noexcept {
        return this->handle_mapper.get(handle).and_then(
            [this](const decltype(handle_mapper)::ValueType& mapped) {
                return this->rigid_bodies.get(mapped.config);
            });
    }

    std::span<const RigidBodyPool::RigidBody> RigidBodyPool::get_rigid_bodies(void) const noexcept {
        return this->rigid_bodies.get_all();
    }
} // namespace enishi::physics::bullet3