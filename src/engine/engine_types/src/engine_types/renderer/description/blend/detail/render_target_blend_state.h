#pragma once
#include "blend_factor.h"
#include "blend_operator.h"
#include "color_white_mask.h"
#include <cstdint>

namespace enishi::types {
    struct RenderTargetBlendState {
        bool enabled;
        BlendFactor src_color;
        BlendFactor dst_color;
        BlendOperator color_operator;
        BlendFactor src_alpha;
        BlendFactor dst_alpha;
        BlendOperator alpha_operator;
        std::uint8_t write_mask;

        static constexpr RenderTargetBlendState default_blend_state(void) {
            return RenderTargetBlendState{
                .enabled = true,
                .src_color = BlendFactor::SrcAlpha,
                .dst_color = BlendFactor::OneMinusSrcAlpha,
                .color_operator = BlendOperator::Add,
                .src_alpha = BlendFactor::One,
                .dst_alpha = BlendFactor::OneMinusSrcAlpha,
                .alpha_operator = BlendOperator::Add,
                .write_mask = static_cast<decltype(RenderTargetBlendState::write_mask)>(
                    ColorWriteMask::ColorWriteRGBA),
            };
        }
    };
} // namespace enishi::types