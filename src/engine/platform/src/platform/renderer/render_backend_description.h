#pragma once
#include <engine_types/renderer/description/view/image_view_description.h>
#include <engine_types/renderer/graphics_api.h>
#include <string>

namespace enishi::platform {
    struct RenderBackendDescription {
        types::GraphicsAPI api;
        std::string shader_suffix;
        types::ImageFormat depth_format;
    };
} // namespace enishi::platform
