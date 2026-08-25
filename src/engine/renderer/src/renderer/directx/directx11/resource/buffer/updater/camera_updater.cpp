#include "camera_updater.h"

namespace enishi::renderer::directx {
    CameraUpdater::CameraUpdater(types::OwnedRenderData&& camera,
        ID3D11Context::Context context,
        INativeBufferAccessor::NativeBuffer buffer)
        : camera(std::move(camera))
        , context(context)
        , buffer(buffer) {
    }

    void CameraUpdater::update(void) {
        const auto render_data = this->camera.get_render_data();

        D3D11_MAPPED_SUBRESOURCE mapped;
        this->context->Map(this->buffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
        memcpy(mapped.pData, render_data.raw_data(), render_data.byte_width());
        this->context->Unmap(this->buffer.Get(), 0);
    }

    types::OwnedRenderData& CameraUpdater::get_resource(void) {
        return this->camera;
    }
} // namespace enishi::renderer::directx