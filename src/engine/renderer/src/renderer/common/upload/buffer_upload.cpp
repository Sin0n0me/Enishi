#include "buffer_upload.h"
#include <cstring>

namespace enishi::renderer {
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

    foundation::Result<void, platform::RenderError> write_vertex_positions(
        platform::IResourceUpdater& updater,
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

    foundation::Result<void, platform::RenderError> write_material_uniform(
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

    foundation::Result<void, platform::RenderError> stage_vertex_uvs(
        platform::IResourceUpdater& updater,
        std::span<const types::MeshHandles::UVStream> streams,
        std::span<const std::vector<glm::vec4>> channels) {
        if (channels.size() > streams.size()) {
            return foundation::Error(platform::RenderError::ResolveError, "UV streams are missing");
        }
        auto& resource = updater.get_resource();
        const auto data = resource.get_render_data();
        for (std::size_t channel = 0; channel < channels.size(); ++channel) {
            const auto& stream = streams[channel];
            if (stream.components == 0 || stream.components > glm::vec4::length()) {
                return foundation::Error(
                    platform::RenderError::ResolveError, "Invalid UV component count");
            }
            const auto width = stream.components * sizeof(float);
            if (data.stride < width || stream.offset > data.stride - width ||
                data.byte_width() % data.stride != 0 ||
                data.byte_width() / data.stride != channels[channel].size()) {
                return foundation::Error(
                    platform::RenderError::ResolveError, "UV layout or count mismatch");
            }
        }
        for (std::size_t channel = 0; channel < channels.size(); ++channel) {
            const auto& stream = streams[channel];
            for (std::size_t vertex = 0; vertex < channels[channel].size(); ++vertex) {
                std::memcpy(&resource[vertex * data.stride + stream.offset],
                    &channels[channel][vertex],
                    stream.components * sizeof(float));
            }
        }
        return {};
    }

} // namespace enishi::renderer
