#pragma once
#include "interface_native_rigid_body_accessor.h"
#include "rigid_body.h"
#include <engine_types/handle/handle_mapper.h>
#include <foundation/pool/resource_pool.h>
#include <memory>
#include <vector>

namespace enishi::physics::bullet3 {
    class RigidBodyPool : public INativeRigidBodyAccessor {
      private:
        struct RigidBodyHandle {
            std::size_t index;
            std::size_t config;
        };

      private:
        types::ResourceMapper<RigidBodyHandle> handle_mapper;
        foundation::ResourcePool<NativeRigidBody> native_rigid_bodies;
        foundation::ResourcePool<RigidBody> rigid_bodies;

      public:
        std::tuple<types::HandleId, NativeRigidBody&> emplace_native_rigid_body(
            NativeRigidBody&& rigid_body) noexcept override;
        void remove_native_rigid_body(const types::HandleId handle) noexcept override;

        foundation::Option<NativeRigidBody&> get_native_rigid_body(
            const types::HandleId handle) noexcept override;
        foundation::Option<const NativeRigidBody&> get_native_rigid_body(
            const types::HandleId handle) const noexcept override;

        std::span<const NativeRigidBody> get_native_rigid_bodies(void) const noexcept override;

        foundation::Option<RigidBody&> link_rigid_body(
            const types::HandleId handle, RigidBody&& rigid_body) noexcept override;
        foundation::Option<RigidBody&> get_rigid_body(
            const types::HandleId handle) noexcept override;
        foundation::Option<const RigidBody&> get_rigid_body(
            const types::HandleId handle) const noexcept override;
        std::span<const RigidBody> get_rigid_bodies(void) const noexcept override;
    };
} // namespace enishi::physics::bullet3
