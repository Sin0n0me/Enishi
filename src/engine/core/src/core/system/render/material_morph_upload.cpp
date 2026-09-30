#include "material_morph_upload.h"
#include <core/system/animation/material_morph.h>
#include <cstring>

namespace enishi::core {
    platform::RenderResult<void> write_material_uniform(
        platform::IResourceUpdater& updater, const types::UniformMaterial& material) {
        auto& resource = updater.get_resource();
        if (resource.get_render_data().byte_width() < sizeof(material)) {
            return foundation::Error(
                platform::RenderError::ResolveError, "Material uniform buffer is too small");
        }
        std::memcpy(&resource[0], &material, sizeof(material));
        updater.on_update();
        return {};
    }

    platform::RenderResult<void> upload_material_morphs(platform::IRenderer& renderer,
        const component::ModelComponent& model,
        const component::MorphComponent& morph) {
        if (model.morph_base_materials.empty()) {
            return {};
        }
        const auto materials = evaluate_material_morphs(
            model.morph_base_materials, model.morph_targets.targets, morph.weights);
        if (materials.is_err()) {
            return materials.propagation(platform::RenderError::ResolveError);
        }
        const auto mapped = renderer.get_handle_mapper()->get(model.render_handle);
        if (mapped.is_none()) {
            return foundation::Error(
                platform::RenderError::ResolveError, "Material morph mesh handle is missing");
        }
        auto* resources = renderer.get_resource_accessor()->get_resource_accessor();
        const auto mesh = resources->get_mesh_accessor()->get_mesh_handle(mapped.unwrap().resource);
        if (mesh.is_none() ||
            mesh.unwrap().material_uniform_buffers.size() != materials.unwrap().size()) {
            return foundation::Error(
                platform::RenderError::ResolveError, "Material morph uniform count mismatch");
        }
        for (std::size_t index = 0; index < materials.unwrap().size(); ++index) {
            const auto& uniforms = mesh.unwrap().material_uniform_buffers[index];
            const auto found = uniforms.find(types::UniformMaterial::UNIFORM_NAME);
            if (found == uniforms.end() || found->second.empty()) {
                return foundation::Error(
                    platform::RenderError::ResolveError, "Material morph uniform is missing");
            }
            for (const auto handle : found->second) {
                const auto buffer = resources->get_buffer_accessor()->get_buffer(handle);
                if (buffer.is_none() || buffer.unwrap() == nullptr) {
                    return foundation::Error(
                        platform::RenderError::ResolveError, "Material morph updater is missing");
                }
                auto result = write_material_uniform(*buffer.unwrap(), materials.unwrap()[index]);
                if (result.is_err()) {
                    return result;
                }
            }
        }
        return {};
    }
} // namespace enishi::core
