#pragma once
#include <engine_types/handle/renderer/handles/mesh_handles.h>
#include <engine_types/handle/renderer/handles/resource_handles.h>
#include <platform/renderer/mesh/interface_mesh_accessor.h>
#include <renderer/common/resource_pool.h>

namespace enishi::renderer {
    class MeshPool : public platform::IMeshAccessor {
      private:
        struct MeshHandle {
            std::size_t resource_index;
            std::size_t interface_index;
        };

      private:
        types::ResourceMapper<MeshHandle> handle_mapper;
        ResourcePool<types::MeshHandles> meshes;

      public:
        std::tuple<types::HandleId, MeshPool::MeshHandles&> make_mesh_handles(
            void) noexcept override;
        void remove_mesh_handles(const types::HandleId handle) noexcept override;
        foundation::Option<MeshPool::MeshHandles&> get_mesh_handle(
            const types::HandleId handle) noexcept override;
        foundation::Option<const MeshPool::MeshHandles&> get_mesh_handle(
            const types::HandleId handle) const noexcept override;
    };
} // namespace enishi::renderer