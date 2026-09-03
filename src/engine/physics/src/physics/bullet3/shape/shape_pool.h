#pragma once
#include "interface_native_shape_accessor.h"
#include "shape.h"
#include <engine_types/handle/handle_mapper.h>
#include <foundation/pool/resource_pool.h>
#include <memory>
#include <vector>

namespace enishi::physics::bullet3 {
    class ShapePool : public INativeShapeAccessor {
      private:
        struct ShapeHandle {
            std::size_t index;
        };

      private:
        types::ResourceMapper<ShapeHandle> handle_mapper;
        foundation::ResourcePool<NativeShape> native_shapes;

      public:
        std::tuple<types::HandleId, NativeShape&> emplace_native_shape(
            NativeShape&& shape) noexcept override;
        std::tuple<types::HandleId, NativeShape&> make_native_shape(void) noexcept override;
        void remove_native_shape(const types::HandleId handle) noexcept override;

        foundation::Option<NativeShape&> get_native_shape(
            const types::HandleId handle) noexcept override;
        foundation::Option<const NativeShape&> get_native_shape(
            const types::HandleId handle) const noexcept override;
    };
} // namespace enishi::physics::bullet3
