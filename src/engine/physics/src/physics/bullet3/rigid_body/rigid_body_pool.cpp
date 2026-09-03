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

    std::tuple<types::HandleId, RigidBodyPool::NativeRigidBody&>
    RigidBodyPool::make_native_rigid_body(void) noexcept {
        return this->handle_mapper.make_from(
            this->native_rigid_bodies.emplace(
                std::make_unique<RigidBodyPool::NativeRigidBody::element_type>()),
            [](const std::size_t index) {
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

    std::span<const RigidBodyPool::NativeRigidBody&> RigidBodyPool::get_native_rigid_bodies(
        void) const noexcept {
        return this->native_rigid_bodies.get_all();
    }
} // namespace enishi::physics::bullet3