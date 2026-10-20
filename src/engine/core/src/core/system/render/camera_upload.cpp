#include "camera_upload.h"

namespace enishi::core {
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
        const auto mapped = renderer.get_handle_mapper()->get(mesh_handle);
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
        const auto found = uniforms.find(types::UniformCamera::UNIFORM_NAME);
        if (found == uniforms.end() || found->second.empty()) {
            return foundation::Error(
                platform::RenderError::ResolveError, "Model camera uniform is missing");
        }
        for (const auto handle : found->second) {
            const auto buffer = resources->get_buffer_accessor()->get_buffer(handle);
            if (buffer.is_none() || buffer.unwrap() == nullptr) {
                return foundation::Error(
                    platform::RenderError::ResolveError, "Camera updater is missing");
            }
            auto result = write_camera(*buffer.unwrap(), camera);
            if (result.is_err()) {
                return result;
            }
        }
        return {};
    }
} // namespace enishi::core
