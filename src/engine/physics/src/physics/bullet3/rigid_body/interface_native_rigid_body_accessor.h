#pragma once
#include <btBulletCollisionCommon.h>
#include <btBulletDynamicsCommon.h>
#include <engine_types/handle/handle_type.h>
#include <foundation/option/option.h>
#include <memory>
#include <span>
#include <tuple>

namespace enishi::physics::bullet3 {
    class INativeRigidBodyAccessor {
      public:
        using NativeRigidBody = std::unique_ptr<btRigidBody>;

      public:
        virtual ~INativeRigidBodyAccessor(void) noexcept = default;

        [[nodiscard]] virtual std::tuple<types::HandleId, NativeRigidBody&>
        emplace_native_rigid_body(NativeRigidBody&& rigid_body) noexcept = 0;
        [[nodiscard]] virtual std::tuple<types::HandleId, NativeRigidBody&> make_native_rigid_body(
            void) noexcept = 0;
        [[nodiscard]] virtual void remove_native_rigid_body(
            const types::HandleId handle) noexcept = 0;

        [[nodiscard]] virtual foundation::Option<NativeRigidBody&> get_native_rigid_body(
            const types::HandleId handle) noexcept = 0;
        [[nodiscard]] virtual foundation::Option<const NativeRigidBody&> get_native_rigid_body(
            const types::HandleId handle) const noexcept = 0;

        [[nodiscard]] virtual std::span<const NativeRigidBody&> get_native_rigid_bodies(
            void) const noexcept = 0;
    };
} // namespace enishi::physics::bullet3