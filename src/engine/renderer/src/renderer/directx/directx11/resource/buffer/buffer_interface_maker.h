#pragma once
#include "interface_native_buffer_accessor.h"
#include <engine_types/handle/renderer/handles/resource_handles.h>
#include <memory>
#include <platform/renderer/buffer/interface_buffer_accessor.h>
#include <platform/renderer/updater/interface_uniform_updater.h>
#include <platform/renderer/view/interface_view_accessor.h>
#include <renderer/common/resource_pool.h>
#include <renderer/directx/directx11/interface_d3d11_context.h>

namespace enishi::renderer::directx {
    class BufferInterfaceMaker {
      public:
        static std::shared_ptr<platform::IResourceUpdater> make_uniform(
            types::OwnedRenderData&& render_data,
            const ID3D11Context::Context context,
            const INativeBufferAccessor::NativeBuffer buffer);
    };
} // namespace enishi::renderer::directx