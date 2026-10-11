#pragma once
#include <engine_types/handle/renderer/render_handle.h>
#include <vector>

namespace enishi::types {
    struct TextureHandlers {
        RenderHandle view_handle;
        std::vector<RenderHandle> sampler_handles;
    };
} // namespace enishi::types
