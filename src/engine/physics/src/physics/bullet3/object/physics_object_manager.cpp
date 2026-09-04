#include "physics_object_manager.h"

namespace enishi::physics::bullet3 {
    types::HandleId PhysicsObjectManager::add_object(void) {
        const auto [handle, _] =
            this->handle_mapper.make_from(this->objects.make(), [](const std::size_t index) {
                return decltype(handle_mapper)::ValueType{
                    .index = index,
                };
            });
        return handle;
    }

    void PhysicsObjectManager::remove_object(const types::HandleId& handle) {
    }

    foundation::Option<std::span<const types::PhysicsHandle>> PhysicsObjectManager::get_handles(
        const types::PhysicsHandle& object_handle,
        const types::PhysicsHandleType& handle_type) const {
        return this->handle_mapper.get(object_handle.id)
            .and_then([&](const decltype(handle_mapper)::ValueType& mapped)
                          -> foundation::Option<std::span<const types::PhysicsHandle>> {
                auto opt_object = this->objects.get(mapped.index);
                if (opt_object.is_none()) {
                    return {};
                }
                auto& pool_map = opt_object.unwrap();
                const auto pool_iter = pool_map.find(handle_type);
                if (pool_iter == pool_map.end()) {
                    return {};
                }

                return std::span{pool_iter->second};
            });
    }

    foundation::VoidResult<PhysicsError> PhysicsObjectManager::link_handle(
        const types::PhysicsHandle& object_handle, const types::PhysicsHandle& link_handle) {
        if (object_handle.type != types::PhysicsHandleType::PhysicsObject) {
            return foundation::Error(PhysicsError::ObjectError, "不正なハンドルです");
        }
        if (!this->object_map.contains(object_handle)) {
            return foundation::Error(PhysicsError::ObjectError, "不明なオブジェクトを参照しました");
        }

        auto& vec = this->object_map[object_handle][link_handle.type];
        const auto index = vec.size();
        vec.emplace_back(link_handle);
        this->index_map[link_handle] = index;

        return {};
    }
} // namespace enishi::physics::bullet3