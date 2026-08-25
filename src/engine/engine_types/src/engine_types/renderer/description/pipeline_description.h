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
    /*
    enum class BlendMode : std::uint8_t {
        Opaque,        // 不透明
        AlphaBlend,    // 通常アルファブレンド
        Additive,      // 加算合成
        Premultiplied, // 乗算済みアルファ
    };

    enum class DepthTestMode : std::uint8_t {
        ReadWrite, // 深度テストあり, 書き込みあり(通常)
        ReadOnly,  // 深度テストあり, 書き込みなし(半透明)
        Disabled,  // 深度テストなし(UI, デバッグ)
    };
    */

    struct PipelineDescription {
        std::vector<RenderHandle> shaders;
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