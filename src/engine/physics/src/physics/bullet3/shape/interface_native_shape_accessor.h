#pragma once
#include <btBulletCollisionCommon.h>
#include <btBulletDynamicsCommon.h>
#include <engine_types/handle/handle_type.h>
#include <foundation/option/option.h>
#include <memory>
#include <tuple>

namespace enishi::physics::bullet3 {
    class INativeShapeAccessor {
      public:
        using NativeShape = std::unique_ptr<btCollisionShape>;

      public:
        virtual ~INativeShapeAccessor(void) noexcept = default;

        [[nodiscard]] virtual std::tuple<types::HandleId, NativeShape&> emplace_native_shape(
            NativeShape&& shape) noexcept = 0;
        [[nodiscard]] virtual std::tuple<types::HandleId, NativeShape&> make_native_shape(
            void) noexcept = 0;
        [[nodiscard]] virtual void remove_native_shape(const types::HandleId handle) noexcept = 0;

        [[nodiscard]] virtual foundation::Option<NativeShape&> get_native_shape(
            const types::HandleId handle) noexcept = 0;
        [[nodiscard]] virtual foundation::Option<const NativeShape&> get_native_shape(
            const types::HandleId handle) const noexcept = 0;
    };
} // namespace enishi::physics::bullet3