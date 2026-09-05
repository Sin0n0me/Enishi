#pragma once
#include <btBulletCollisionCommon.h>
#include <btBulletDynamicsCommon.h>
#include <engine_types/handle/physics/physics_handle.h>
#include <foundation/option/option.h>
#include <memory>
#include <platform/physics/interface_physics_handle_mapper.h>

namespace enishi::physics::bullet3 {
    class PhysicsHandleMapper : public platform::IPhysicsHandleMapper {
      private:
        std::unique_ptr<types::HandleAllocator> handle_allocator;
        std::unordered_map<types::PhysicsHandle, types::ResourceHandles> handle_mapper;

      public:
        PhysicsHandleMapper(void);

        types::PhysicsHandle make(
            const types::PhysicsHandleType& handle_type, types::ResourceHandles&& mapped) noexcept;

        foundation::Option<types::ResourceHandles&> get(
            const types::PhysicsHandle& handle) noexcept;

        types::ResourceHandles& operator[](const types::PhysicsHandle& handle);

      public:
        foundation::Option<const types::ResourceHandles&> get(
            const types::PhysicsHandle& handle) const noexcept override;
    };
} // namespace enishi::physics::bullet3