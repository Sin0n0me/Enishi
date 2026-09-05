#pragma once
#include "../interface_native_buffer_accessor.h"
#include <platform/renderer/updater/interface_uniform_updater.h>
#include <renderer/directx/directx11/interface_d3d11_context.h>

namespace enishi::renderer::directx {
    class BoneUpdater : public platform::IUnifromUpdater {
      private:
        types::OwnedRenderData render_data;
        ID3D11Context::Context context;
        INativeBufferAccessor::NativeBuffer buffer;

      public:
        BoneUpdater(types::OwnedRenderData&& render_data,
            const ID3D11Context::Context context,
            const INativeBufferAccessor::NativeBuffer buffer);

        void on_update(void) override;
        types::OwnedRenderData& get_resource(void) override;
    };
} // namespace enishi::renderer::directx