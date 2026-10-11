#include "vertex_buffer_updater.h"

namespace enishi::renderer {
    VertexBufferUpdater::VertexBufferUpdater(
        types::OwnedRenderData&& resource, std::function<void(const types::RenderData&)> upload)
        : resource(std::move(resource))
        , upload(std::move(upload)) {
    }

    void VertexBufferUpdater::on_update(void) {
        this->upload(this->resource.get_render_data());
    }

    types::OwnedRenderData& VertexBufferUpdater::get_resource(void) {
        return this->resource;
    }
} // namespace enishi::renderer
