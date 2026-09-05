#pragma once
#include <btBulletCollisionCommon.h>
#include <btBulletDynamicsCommon.h>
#include <engine_types/handle/handle_type.h>
#include <foundation/option/option.h>
#include <foundation/type_traits/add_const.h>
#include <memory>
#include <platform/physics/rigid_body/interface_rigid_body.h>
#include <span>
#include <tuple>

namespace enishi::physics::bullet3 {
    class INativeRigidBodyAccessor {
      public:
        using NativeRigidBody = std::shared_ptr<btRigidBody>;
        using RigidBody = std::shared_ptr<platform::IRigidBody>;

      public:
        virtual ~INativeRigidBodyAccessor(void) noexcept = default;

        [[nodiscard]] virtual std::tuple<types::HandleId, NativeRigidBody&>
        emplace_native_rigid_body(NativeRigidBody&& rigid_body) noexcept = 0;
        [[nodiscard]] virtual void remove_native_rigid_body(
            const types::HandleId handle) noexcept = 0;

        [[nodiscard]] virtual foundation::Option<NativeRigidBody&> get_native_rigid_body(
            const types::HandleId handle) noexcept = 0;
        [[nodiscard]] virtual foundation::Option<const NativeRigidBody&> get_native_rigid_body(
            const types::HandleId handle) const noexcept = 0;

        [[nodiscard]] virtual std::span<const NativeRigidBody> get_native_rigid_bodies(
            void) const noexcept = 0;

        [[nodiscard]] virtual foundation::Option<RigidBody&> link_rigid_body(
            const types::HandleId handle, RigidBody&& rigid_body) noexcept = 0;
        [[nodiscard]] virtual foundation::Option<RigidBody&> get_rigid_body(
            const types::HandleId handle) noexcept = 0;
        [[nodiscard]] virtual foundation::Option<const RigidBody&> get_rigid_body(
            const types::HandleId handle) const noexcept = 0;
        [[nodiscard]] virtual std::span<const RigidBody> get_rigid_bodies(void) const noexcept = 0;
    };
} // namespace enishi::physics::bullet3