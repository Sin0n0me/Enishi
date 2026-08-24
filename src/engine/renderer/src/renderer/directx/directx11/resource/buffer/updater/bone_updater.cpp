#include "bone_updater.h"

namespace enishi::renderer::directx {
    BoneUpdater::BoneUpdater(Microsoft::WRL::ComPtr<ID3D11DeviceContext> context)
        : context(context) {
    }

    void BoneUpdater::update(void) {
        const auto render_data = this->bones.get_render_data();

        D3D11_MAPPED_SUBRESOURCE mapped;
        this->context->Map(this->buffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
        memcpy(mapped.pData, render_data.raw_data(), render_data.byte_width());
        this->context->Unmap(this->buffer.Get(), 0);
    }

    void BoneUpdater::set_resource(types::OwnedRenderData&& resource) {
        return this->bones = resource;
    }

    types::OwnedRenderData& BoneUpdater::get_resource(void) {
        return this->bones;
    }
} // namespace enishi::renderer::directx