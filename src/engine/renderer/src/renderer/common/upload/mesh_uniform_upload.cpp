#include "mesh_uniform_upload.h"
namespace enishi::renderer {
    platform::RenderResult<void> update_mesh_uniform(platform::IRenderer& renderer,
        types::RenderHandle mesh_handle,
        const std::string& name,
        const UniformWriter& write,
        const char* missing_uniform,
        const char* missing_updater,
        bool require_nonempty) {
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
        const auto found = uniforms.find(name);
        if (found == uniforms.end() || (require_nonempty && found->second.empty())) {
            return foundation::Error(platform::RenderError::ResolveError, missing_uniform);
        }
        for (const auto handle : found->second) {
            const auto buffer = resources->get_buffer_accessor()->get_buffer(handle);
            if (buffer.is_none() || buffer.unwrap() == nullptr) {
                return foundation::Error(platform::RenderError::ResolveError, missing_updater);
            }
            auto result = write(*buffer.unwrap());
            if (result.is_err()) {
                return result;
            }
        }
        return {};
    }
} // namespace enishi::renderer
