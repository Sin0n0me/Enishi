#include "uv_morph_upload.h"
#include <cstring>

namespace enishi::core {
    foundation::Result<void, platform::RenderError> stage_vertex_uvs(
        platform::IResourceUpdater& updater,
        std::span<const types::MeshHandles::UVStream> streams,
        const UVChannels& channels) {
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
} // namespace enishi::core
