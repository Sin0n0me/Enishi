#include "shape_pool.h"

namespace enishi::physics::bullet3 {
    std::tuple<types::HandleId, ShapePool::NativeShape&> ShapePool::emplace_native_shape(
        NativeShape&& shape) noexcept {
        return this->handle_mapper.make_from(
            this->native_shapes.emplace(std::move(shape)), [](const std::size_t index) {
                return decltype(handle_mapper)::ValueType{
                    .index = index,
                };
            });
    }

    std::tuple<types::HandleId, ShapePool::NativeShape&> ShapePool::make_native_shape(
        void) noexcept {
        return this->handle_mapper.make_from(
            this->native_shapes.emplace(std::make_unique<ShapePool::NativeShape::element_type>()),
            [](const std::size_t index) {
                return decltype(handle_mapper)::ValueType{
                    .index = index,
                };
            });
    }

    void ShapePool::remove_native_shape(const types::HandleId handle) noexcept {
    }

    foundation::Option<ShapePool::NativeShape&> ShapePool::get_native_shape(
        const types::HandleId handle) noexcept {
        return this->handle_mapper.get(handle).and_then(
            [this](const decltype(handle_mapper)::ValueType& mapped) {
                return this->native_shapes.get(mapped.index);
            });
    }

    foundation::Option<const ShapePool::NativeShape&> ShapePool::get_native_shape(
        const types::HandleId handle) const noexcept {
        return this->handle_mapper.get(handle).and_then(
            [this](const decltype(handle_mapper)::ValueType& mapped) {
                return this->native_shapes.get(mapped.index);
            });
    }
} // namespace enishi::physics::bullet3