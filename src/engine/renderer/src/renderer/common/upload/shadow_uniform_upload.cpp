#include "shadow_uniform_upload.h"
#include "mesh_uniform_upload.h"
namespace enishi::renderer {
    foundation::Result<void, platform::RenderError> write_shadow(
        platform::IResourceUpdater& updater, const types::UniformShadow& shadow) {
        auto& resource = updater.get_resource();
        const auto data = resource.get_render_data();
        if (data.stride != sizeof(types::UniformShadow) ||
            data.byte_width() != sizeof(types::UniformShadow)) {
            return foundation::Error(
                platform::RenderError::ResolveError, "Shadow buffer capacity or stride mismatch");
        }
        resource.update(shadow, 0);
        updater.on_update();
        return {};
    }

    foundation::Result<void, platform::RenderError> upload_model_shadow(
        platform::IRenderer& renderer,
        types::RenderHandle mesh_handle,
        const types::UniformShadow& shadow) {
        return update_mesh_uniform(
            renderer,
            mesh_handle,
            types::UniformShadow::UNIFORM_NAME,
            [&shadow](
                platform::IResourceUpdater& updater) { return write_shadow(updater, shadow); },
            "Model shadow uniform is missing",
            "Shadow updater is missing");
    }
} // namespace enishi::renderer
