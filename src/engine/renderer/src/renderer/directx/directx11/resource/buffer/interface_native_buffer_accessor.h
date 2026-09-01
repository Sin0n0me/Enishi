#pragma once
#include <d3d11.h>
#include <engine_types/handle/handle_type.h>
#include <foundation/option/option.h>
#include <memory>
#include <platform/renderer/updater/interface_resource_updater.h>
#include <tuple>
#include <wrl/client.h>

namespace enishi::renderer::directx {
    class INativeBufferAccessor {
      public:
        using BufferInterface = std::shared_ptr<platform::IResourceUpdater>;
        using NativeBuffer = Microsoft::WRL::ComPtr<ID3D11Buffer>;

      public:
        virtual ~INativeBufferAccessor(void) noexcept = default;

        [[nodiscard]] virtual std::tuple<types::HandleId, NativeBuffer&> make_native_buffer(
            void) noexcept = 0;
        [[nodiscard]] virtual void remove_native_buffer(const types::HandleId handle) noexcept = 0;
        [[nodiscard]] virtual foundation::Option<NativeBuffer&> get_native_buffer(
            const types::HandleId handle) noexcept = 0;
        [[nodiscard]] virtual foundation::Option<const NativeBuffer&> get_native_buffer(
            const types::HandleId handle) const noexcept = 0;

        virtual void add_interface(
            const types::HandleId handle, const BufferInterface buffer_interface) noexcept = 0;
        [[nodiscard]] virtual foundation::Option<BufferInterface&> get_buffer_interface(
            const types::HandleId handle) noexcept = 0;
        [[nodiscard]] virtual foundation::Option<const BufferInterface&> get_buffer_interface(
            const types::HandleId handle) const noexcept = 0;
    };
} // namespace enishi::renderer::directx