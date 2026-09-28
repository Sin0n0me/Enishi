#pragma once
#include <engine_types/handle/renderer/render_handle.h>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace enishi::types {
    struct MeshHandles {
        std::vector<RenderHandle> mesh_handles;
        using UniformBuffers = std::unordered_map<std::string, std::vector<HandleId>>;
        UniformBuffers uniform_buffers;
        struct PositionStream {
            HandleId buffer;
            std::size_t offset;
        };
        std::optional<PositionStream> positions;
        struct UVStream {
            HandleId buffer;
            std::size_t offset;
            std::size_t components;
        };
        std::vector<UVStream> uvs;
    };
} // namespace enishi::types
