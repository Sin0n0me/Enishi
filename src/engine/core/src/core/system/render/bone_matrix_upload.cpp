#include "bone_matrix_upload.h"
#include <engine_types/renderer/uniform_buffer/bones.h>

namespace enishi::core {
    foundation::Result<void, platform::RenderError> write_bone_matrices(
        platform::IResourceUpdater& updater, std::span<const glm::mat4> matrices) {
        auto& resource = updater.get_resource();
        const auto data = resource.get_render_data();
        if (data.stride != sizeof(glm::mat4) || data.byte_width() < matrices.size_bytes()) {
            return foundation::Error(
                platform::RenderError::ResolveError, "Bone buffer capacity or stride mismatch");
        }
        for (std::size_t index = 0; index < matrices.size(); ++index) {
            resource.update(matrices[index], index);
        }
        updater.on_update();
        return {};
    }

    foundation::Result<void, platform::RenderError> upload_bone_matrices(
        platform::IRenderer& renderer,
        const component::ModelComponent& model,
        const component::SkinningComponent& skinning) {
        const auto mapped = renderer.get_handle_mapper()->get(model.render_handle);
        if (mapped.is_none()) {
            return foundation::Error(
                platform::RenderError::ResolveError, "Model mesh handle is missing");
        }
        auto* resources = renderer.get_resource_accessor()->get_resource_accessor();
        const auto mesh = resources->get_mesh_accessor()->get_mesh_handle(mapped.unwrap().resource);
        if (mesh.is_none()) {
            return foundation::Error(
                platform::RenderError::ResolveError, "Model mesh resource is missing");
        }
        const auto& uniforms = mesh.unwrap().uniform_buffers;
        const auto found = uniforms.find(types::MediumModelBones::UNIFORM_NAME);
        if (found == uniforms.end()) {
            return foundation::Error(
                platform::RenderError::ResolveError, "Model bone uniform is missing");
        }
        for (const auto handle : found->second) {
            const auto buffer = resources->get_buffer_accessor()->get_buffer(handle);
            if (buffer.is_none() || buffer.unwrap() == nullptr) {
                return foundation::Error(
                    platform::RenderError::ResolveError, "Bone updater is missing");
            }
            auto result = write_bone_matrices(*buffer.unwrap(), skinning.skinning_matrices);
            if (result.is_err()) {
                return result;
            }
        }
        return {};
    }
} // namespace enishi::core
