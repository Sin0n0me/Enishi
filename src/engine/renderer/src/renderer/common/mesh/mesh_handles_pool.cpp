#include "mesh_handles_pool.h"

namespace enishi::renderer {
    std::tuple<types::HandleId, MeshPool::MeshHandles&> MeshPool::make_mesh_handles(void) noexcept {
        return this->handle_mapper.make_from(this->meshes.make(), [](const std::size_t index) {
            return decltype(handle_mapper)::ValueType{
                .resource_index = index,
            };
        });
    }

    void MeshPool::remove_mesh_handles(const types::HandleId handle) noexcept {
    }

    foundation::Option<MeshPool::MeshHandles&> MeshPool::get_mesh_handle(
        const types::HandleId handle) noexcept {
        return this->handle_mapper.get(handle).and_then(
            [this](const decltype(handle_mapper)::ValueType mapped_handle) {
                return this->meshes.get(mapped_handle.resource_index);
            });
    }

    foundation::Option<const MeshPool::MeshHandles&> MeshPool::get_mesh_handle(
        const types::HandleId handle) const noexcept {
        return this->handle_mapper.get(handle).and_then(
            [this](const decltype(handle_mapper)::ValueType mapped_handle) {
                return this->meshes.get(mapped_handle.resource_index);
            });
    }
} // namespace enishi::renderer