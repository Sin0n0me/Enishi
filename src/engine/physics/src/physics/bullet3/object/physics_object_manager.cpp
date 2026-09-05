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
        return this->get_map(object_handle)
            .and_then([&](const HandlePoolMap& pool_map)
                          -> foundation::Option<std::span<const types::PhysicsHandle>> {
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
        auto opt_pool_map = this->get_map(object_handle);
        if (opt_pool_map.is_none()) {
            return foundation::Error(PhysicsError::ObjectError, "不明なオブジェクトを参照しました");
        }
        auto& pool_map = opt_pool_map.unwrap_mut();

        auto& vec = pool_map[link_handle.type];
        vec.emplace_back(link_handle);

        return {};
    }

    foundation::Option<const PhysicsObjectManager::HandlePoolMap&> PhysicsObjectManager::get_map(
        const types::PhysicsHandle& object_handle) const {
        return this->handle_mapper.get(object_handle.id)
            .and_then([&](const decltype(handle_mapper)::ValueType& mapped) {
                return this->objects.get(mapped.index);
            });
    }

    foundation::Option<PhysicsObjectManager::HandlePoolMap&> PhysicsObjectManager::get_map(
        const types::PhysicsHandle& object_handle) {
        return this->handle_mapper.get(object_handle.id)
            .and_then([&](const decltype(handle_mapper)::ValueType& mapped) {
                return this->objects.get(mapped.index);
            });
    }
} // namespace enishi::physics::bullet3