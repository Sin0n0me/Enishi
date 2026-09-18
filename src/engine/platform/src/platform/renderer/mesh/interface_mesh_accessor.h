#pragma once
#include <engine_types/handle/handle_type.h>
#include <engine_types/handle/renderer/handles/mesh_handles.h>
#include <foundation/option/option.h>
#include <span>
#include <tuple>

namespace enishi::platform {
    class IMeshAccessor {
      public:
        using MeshHandles = types::MeshHandles;

      public:
        virtual ~IMeshAccessor(void) noexcept = default;

        [[nodiscard]] virtual std::tuple<types::HandleId, MeshHandles&> make_mesh_handles(
            void) noexcept = 0;
        [[nodiscard]] virtual void remove_mesh_handles(const types::HandleId handle) noexcept = 0;
        [[nodiscard]] virtual foundation::Option<MeshHandles&> get_mesh_handle(
            const types::HandleId handle) noexcept = 0;
        [[nodiscard]] virtual foundation::Option<const MeshHandles&> get_mesh_handle(
            const types::HandleId handle) const noexcept = 0;
    };
} // namespace enishi::platform