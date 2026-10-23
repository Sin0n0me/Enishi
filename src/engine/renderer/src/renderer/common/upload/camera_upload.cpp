#include "camera_upload.h"
#include "mesh_uniform_upload.h"

namespace enishi::renderer {
    foundation::Result<void, platform::RenderError> write_camera(
        platform::IResourceUpdater& updater, const types::UniformCamera& camera) {
        auto& resource = updater.get_resource();
        const auto data = resource.get_render_data();
        if (data.stride != sizeof(types::UniformCamera) ||
            data.byte_width() != sizeof(types::UniformCamera)) {
            return foundation::Error(
                platform::RenderError::ResolveError, "Camera buffer capacity or stride mismatch");
        }
        resource.update(camera, 0);
        updater.on_update();
        return {};
    }

    foundation::Result<void, platform::RenderError> upload_camera(platform::IRenderer& renderer,
        types::RenderHandle mesh_handle,
        const types::UniformCamera& camera) {
        return update_mesh_uniform(
            renderer,
            mesh_handle,
            types::UniformCamera::UNIFORM_NAME,
            [&camera](
                platform::IResourceUpdater& updater) { return write_camera(updater, camera); },
            "Model camera uniform is missing",
            "Camera updater is missing");
    }
} // namespace enishi::renderer
