#pragma once
#include <cstdint>
#include <engine_types/handle/renderer/render_handle.h>
#include <engine_types/renderer/description/blend/blend_description.h>
#include <engine_types/renderer/description/depth/depth_stencil_description.h>
#include <engine_types/renderer/description/rasterizer/rasterizer_description.h>
#include <engine_types/renderer/description/sampler/sampler_description.h>
#include <engine_types/renderer/topology/topology.h>
#include <engine_types/renderer/vertex/vertex_layout.h>

namespace enishi::types {
    struct PipelineDescription {
        std::vector<RenderHandle> shaders;
        std::vector<RenderHandle> shader_reflections;
        PrimitiveTopology topology;
        RenderHandle vertex_layout;
        RenderHandle render_target_view;
        RenderHandle depth_stencil_view;
        RenderHandle rasterizer_state;
        RenderHandle sampler_state;
        RenderHandle depth_stencil_state;
        RenderHandle blend_state;
    };
} // namespace enishi::types