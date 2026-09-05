#pragma once
#include <engine_types/handle/handle_allocator.h>
#include <engine_types/handle/handle_mapper.h>
#include <engine_types/handle/physics/physics_handle.h>
#include <engine_types/physics/object/physics_object.h>
#include <foundation/option/option.h>
#include <foundation/pool/resource_pool.h>
#include <foundation/result/result.h>
#include <memory>
#include <physics/errors/errors.h>
#include <platform/physics/interface_physics_world.h>
#include <unordered_map>

namespace enishi::physics::bullet3 {
    class PhysicsObjectManager {
      private:
        struct PhysicsObjectHandle {
            std::size_t index;
        };

      private:
        using HandlePool = std::vector<types::PhysicsHandle>;
        using HandlePoolMap = std::unordered_map<types::PhysicsHandleType, HandlePool>;

        types::ResourceMapper<PhysicsObjectHandle> handle_mapper;
        foundation::ResourcePool<HandlePoolMap> objects;

      public:
        [[nodiscard]] types::HandleId add_object(void);

        void remove_object(const types::HandleId& handle);

        [[nodiscard]] foundation::Option<std::span<const types::PhysicsHandle>> get_handles(
            const types::PhysicsHandle& object_handle,
            const types::PhysicsHandleType& handle_type) const;

        [[nodiscard]] foundation::VoidResult<PhysicsError> link_handle(
            const types::PhysicsHandle& object_handle, const types::PhysicsHandle& link_handle);

      private:
        [[nodiscard]] foundation::Option<const HandlePoolMap&> get_map(
            const types::PhysicsHandle& object_handle) const;
        [[nodiscard]] foundation::Option<HandlePoolMap&> get_map(
            const types::PhysicsHandle& object_handle);
    };
} // namespace enishi::physics::bullet3