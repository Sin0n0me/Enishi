#include "bone_updater.h"
#include <glm/glm.hpp>

namespace enishi::renderer::directx {
    BoneUpdater::BoneUpdater(types::OwnedRenderData&& render_data,
        const ID3D11Context::Context context,
        const INativeBufferAccessor::NativeBuffer buffer)
        : render_data(std::move(render_data))
        , context(context)
        , buffer(buffer) {
    }

    void BoneUpdater::on_update(void) {
        /*
        for (auto i = 0; i < 256; ++i) {
            this->render_data.update(glm::mat4(1.0f), i);
        }
        */

        const auto bone_matrices = this->render_data.get_render_data();

        D3D11_MAPPED_SUBRESOURCE mapped;
        const auto hr =
            this->context->Map(this->buffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
        if (SUCCEEDED(hr)) {
            memcpy(mapped.pData, bone_matrices.raw_data(), bone_matrices.byte_width());
            this->context->Unmap(this->buffer.Get(), 0);
        }
    }

    types::OwnedRenderData& BoneUpdater::get_resource(void) {
        return this->render_data;
    }
} // namespace enishi::renderer::directx