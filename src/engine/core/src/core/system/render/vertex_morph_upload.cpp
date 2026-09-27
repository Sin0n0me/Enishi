#include "vertex_morph_upload.h"
#include <core/system/animation/vertex_morph.h>
#include <cstring>

namespace enishi::core {
    platform::RenderResult<void> write_vertex_positions(platform::IResourceUpdater& updater,
        std::size_t offset,
        std::span<const glm::vec3> positions) {
        auto& resource = updater.get_resource();
        const auto data = resource.get_render_data();
        if (data.stride < sizeof(glm::vec3) || offset > data.stride - sizeof(glm::vec3) ||
            data.byte_width() % data.stride != 0 ||
            data.byte_width() / data.stride != positions.size()) {
            return foundation::Error(
                platform::RenderError::ResolveError, "Vertex position layout or count mismatch");
        }
        for (std::size_t index = 0; index < positions.size(); ++index) {
            std::memcpy(
                &resource[index * data.stride + offset], &positions[index], sizeof(glm::vec3));
        }
        updater.on_update();
        return {};
    }

    platform::RenderResult<void> upload_vertex_morphs(platform::IRenderer& renderer,
        const component::ModelComponent& model,
        const component::MorphComponent& morph) {
        const auto positions = evaluate_vertex_morphs(
            model.morph_base_positions, model.morph_targets.targets, morph.weights);
        if (positions.is_err()) {
            return positions.propagation(platform::RenderError::ResolveError);
        }
        const auto mapped = renderer.get_handle_mapper()->get(model.render_handle);
        if (mapped.is_none()) {
            return foundation::Error(
                platform::RenderError::ResolveError, "Morph mesh handle is missing");
        }
        auto* resources = renderer.get_resource_accessor()->get_resource_accessor();
        const auto mesh = resources->get_mesh_accessor()->get_mesh_handle(mapped.unwrap().resource);
        if (mesh.is_none() || !mesh.unwrap().positions.has_value()) {
            return foundation::Error(
                platform::RenderError::ResolveError, "Morph vertex stream is missing");
        }
        const auto& stream = mesh.unwrap().positions.value();
        const auto updater = resources->get_buffer_accessor()->get_buffer(stream.buffer);
        if (updater.is_none() || updater.unwrap() == nullptr) {
            return foundation::Error(
                platform::RenderError::ResolveError, "Morph vertex updater is missing");
        }
        return write_vertex_positions(*updater.unwrap(), stream.offset, positions.unwrap());
    }
} // namespace enishi::core
