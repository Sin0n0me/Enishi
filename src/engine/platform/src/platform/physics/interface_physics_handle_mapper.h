#pragma once
#include <engine_types/handle/physics/physics_handle.h>
#include <engine_types/handle/resource_handles.h>
#include <engine_types/renderer/binding_description.h>
#include <foundation/option/option.h>

namespace enishi::platform {
    class IPhysicsHandleMapper {
      public:
        virtual ~IPhysicsHandleMapper(void) noexcept = default;

        [[nodiscard]] virtual foundation::Option<const types::ResourceHandles&> get(
            const types::PhysicsHandle& handle) const noexcept = 0;
    };
} // namespace enishi::platform
