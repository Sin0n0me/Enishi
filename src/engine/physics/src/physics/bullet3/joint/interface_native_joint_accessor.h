#pragma once
#include <btBulletCollisionCommon.h>
#include <btBulletDynamicsCommon.h>
#include <engine_types/handle/handle_type.h>
#include <foundation/option/option.h>
#include <memory>
#include <span>
#include <tuple>

namespace enishi::physics::bullet3 {
    class INativeJointAccessor {
      public:
        using NativeJoint = std::unique_ptr<btTypedConstraint>;

      public:
        virtual ~INativeJointAccessor(void) noexcept = default;

        [[nodiscard]] virtual std::tuple<types::HandleId, NativeJoint&> emplace_native_joint(
            NativeJoint&& joint) noexcept = 0;
        [[nodiscard]] virtual void remove_native_joint(const types::HandleId handle) noexcept = 0;

        [[nodiscard]] virtual foundation::Option<NativeJoint&> get_native_joint(
            const types::HandleId handle) noexcept = 0;
        [[nodiscard]] virtual foundation::Option<const NativeJoint&> get_native_joint(
            const types::HandleId handle) const noexcept = 0;

        [[nodiscard]] virtual std::span<const NativeJoint> get_native_joints(
            void) const noexcept = 0;
    };
} // namespace enishi::physics::bullet3