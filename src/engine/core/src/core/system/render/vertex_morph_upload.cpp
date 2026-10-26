#include "vertex_morph_upload.h"
#include <animation/morph/uv_morph.h>
#include <animation/morph/vertex_morph.h>
#include <cstring>
#include <renderer/common/upload/buffer_upload.h>

namespace enishi::core {
    platform::RenderResult<void> upload_vertex_morphs(platform::IRenderer& renderer,
        const component::ModelComponent& model,
        const component::MorphComponent& morph) {
        const auto positions = animation::evaluate_vertex_morphs(
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
        const auto uvs = animation::evaluate_uv_morphs(
            model.morph_base_uvs, model.morph_targets.targets, morph.weights);
        if (uvs.is_err()) {
            return uvs.propagation(platform::RenderError::ResolveError);
        }
        if (!uvs.unwrap().empty()) {
            for (const auto& uv : mesh.unwrap().uvs) {
                if (uv.buffer != stream.buffer) {
                    return foundation::Error(platform::RenderError::ResolveError,
                        "Morph UV and position buffers differ");
                }
            }
            auto staged =
                renderer::stage_vertex_uvs(*updater.unwrap(), mesh.unwrap().uvs, uvs.unwrap());
            if (staged.is_err()) {
                return staged;
            }
        }
        return renderer::write_vertex_positions(
            *updater.unwrap(), stream.offset, positions.unwrap());
    }
} // namespace enishi::core
