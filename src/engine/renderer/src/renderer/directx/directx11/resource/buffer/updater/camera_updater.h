#pragma once
#include "../interface_native_buffer_accessor.h"
#include <platform/renderer/updater/interface_uniform_updater.h>
#include <renderer/directx/directx11/interface_d3d11_context.h>

namespace enishi::renderer::directx {
    class CameraUpdater : public platform::IUnifromUpdater {
      private:
        ID3D11Context::Context context;
        types::OwnedRenderData camera;
        INativeBufferAccessor::NativeBuffer buffer;

      public:
        CameraUpdater(types::OwnedRenderData&& camera,
            ID3D11Context::Context context,
            INativeBufferAccessor::NativeBuffer buffer);

        void update(void) override;
        types::OwnedRenderData& get_resource(void) override;
    };
} // namespace enishi::renderer::directx