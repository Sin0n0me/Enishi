#include "bone_matrix_upload.h"
#include <engine_types/renderer/uniform_buffer/bones.h>
#include <renderer/common/upload/buffer_upload.h>
#include <renderer/common/upload/mesh_uniform_upload.h>

namespace enishi::core {
    foundation::Result<void, platform::RenderError> upload_bone_matrices(
        platform::IRenderer& renderer,
        const component::ModelComponent& model,
        const component::SkinningComponent& skinning) {
        return renderer::update_mesh_uniform(
            renderer,
            model.render_handle,
            types::MediumModelBones::UNIFORM_NAME,
            [&skinning](platform::IResourceUpdater& updater) {
                return renderer::write_bone_matrices(updater, skinning.skinning_matrices);
            },
            "Model bone uniform is missing",
            "Bone updater is missing",
            false);
    }
} // namespace enishi::core
