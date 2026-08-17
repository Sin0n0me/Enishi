#pragma once
#include "../../handle/renderer/render_handle.h"
#include "../vertex/vertex_layout.h"
#include "rasterizer_description.h"
#include <cstdint>

namespace enishi::types {
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

    enum class PrimitiveTopology : std::uint8_t {
        TriangleList, // 三角形リスト
        LineList,     // 線分リスト
        PointList,    // 点リスト
    };

    struct PipelineDescription {
        std::vector<RenderHandle> shaders;
        RenderHandle vertex_layout;
        RenderHandle rasterizer;
        RenderHandle render_target;
        BlendMode blend_mode;
        DepthTestMode depth_test;
        PrimitiveTopology topology;

        [[nodiscard]]
        static constexpr PipelineDescription make_opaque(RenderHandle vs,
            RenderHandle fs,
            const RenderHandle& layout,
            const RenderHandle& rasterizer) noexcept {
            return PipelineDescription{
                .shaders = {vs, fs},
                .vertex_layout = layout,
                .rasterizer = rasterizer,
                .blend_mode = BlendMode::Opaque,
                .depth_test = DepthTestMode::ReadWrite,
                .topology = PrimitiveTopology::TriangleList,
            };
        }

        [[nodiscard]]
        static PipelineDescription make_shadow(const RenderHandle& vs,
            const RenderHandle& fs,
            const RenderHandle& layout,
            const RenderHandle& rasterizer) noexcept {
            return PipelineDescription{
                .shaders = {vs, fs},
                .vertex_layout = layout,
                .rasterizer = rasterizer,
                .depth_test = DepthTestMode::ReadWrite,
                .topology = PrimitiveTopology::TriangleList,
            };
        }

        [[nodiscard]]
        static PipelineDescription make_wireframe(const RenderHandle& vs,
            const RenderHandle& fs,
            const RenderHandle& layout,
            const RenderHandle& rasterizer) noexcept {
            return PipelineDescription{
                .shaders = {vs, fs},
                .vertex_layout = layout,
                .rasterizer = rasterizer,
                .depth_test = DepthTestMode::ReadOnly,
                .topology = PrimitiveTopology::TriangleList,
            };
        }
    };
} // namespace enishi::types