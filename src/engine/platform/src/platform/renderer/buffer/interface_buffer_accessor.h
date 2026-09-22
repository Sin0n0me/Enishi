#pragma once
#include <engine_types/handle/handle_type.h>
#include <engine_types/handle/renderer/handles/mesh_handles.h>
#include <foundation/option/option.h>
#include <memory>
#include <platform/renderer/updater/interface_resource_updater.h>
#include <tuple>

namespace enishi::platform {
    class IBufferAccessor {
      public:
        using Buffer = std::shared_ptr<IResourceUpdater>;

      public:
        virtual ~IBufferAccessor(void) noexcept = default;

        [[nodiscard]] virtual std::tuple<types::HandleId, Buffer&> make_buffer(void) noexcept = 0;
        [[nodiscard]] virtual void add_interface(
            const types::HandleId handle, const Buffer buffer_interface) noexcept = 0;
        [[nodiscard]] virtual void remove_buffer(const types::HandleId handle) noexcept = 0;
        [[nodiscard]] virtual foundation::Option<Buffer&> get_buffer(
            const types::HandleId handle) noexcept = 0;
        [[nodiscard]] virtual foundation::Option<const Buffer&> get_bufer(
            const types::HandleId handle) const noexcept = 0;
    };
} // namespace enishi::platform