#pragma once
#include "../interface_native_buffer_accessor.h"
#include <platform/renderer/updater/interface_uniform_updater.h>
#include <renderer/directx/directx11/interface_d3d11_context.h>

namespace enishi::renderer::directx {
    class BoneUpdater : public platform::IUnifromUpdater {
      private:
        ID3D11Context::Context context;
        types::OwnedRenderData bones;
        INativeBufferAccessor::NativeBuffer buffer;

      public:
        BoneUpdater(types::OwnedRenderData&& bones,
            ID3D11Context::Context context,
            INativeBufferAccessor::NativeBuffer buffer);

        void on_update(void) override;
        types::OwnedRenderData& get_resource(void) override;
    };
} // namespace enishi::renderer::directx