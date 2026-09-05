#pragma once
#include "detail/rasterizer_mode.h"
#include <cstdint>

namespace enishi::types {
    struct DepthBiasDescription {
        bool enable;
        float constant_factor; // 固定オフセット
        float slope_factor;    // 傾きに応じたオフセット
        float clamp;           // バイアスの最大値
    };

    struct RasterizerStateDescription {
        FillMode fill_mode;
        CullMode cull_mode;
        FrontFace front_face;
        bool depth_clamp; // 深度値をクランプ
        float line_width; // Wireframe時の線幅
        DepthBiasDescription depth_bias;

        [[nodiscard]]
        static constexpr RasterizerStateDescription default_solid(void) noexcept {
            return RasterizerStateDescription{
                .fill_mode = FillMode::Solid,
                .cull_mode = CullMode::Back,
                .front_face = FrontFace::CounterClockwise,
                .depth_clamp = false,
                .line_width = 1.0f,
                .depth_bias =
                    {
                        .enable = false,
                        .constant_factor = 0.0f,
                        .slope_factor = 0.0f,
                        .clamp = 0.0f,
                    },
            };
        }

        [[nodiscard]]
        static constexpr RasterizerStateDescription wireframe(void) noexcept {
            RasterizerStateDescription desc = RasterizerStateDescription::default_solid();
            desc.fill_mode = FillMode::Wireframe;
            desc.cull_mode = CullMode::None;
            return desc;
        }

        [[nodiscard]]
        static constexpr RasterizerStateDescription no_cull(void) noexcept {
            RasterizerStateDescription desc = RasterizerStateDescription::default_solid();
            desc.cull_mode = CullMode::None;
            return desc;
        }

        // シャドウマップ用
        [[nodiscard]]
        static constexpr RasterizerStateDescription shadow_map(void) noexcept {
            RasterizerStateDescription desc = RasterizerStateDescription::default_solid();
            desc.cull_mode = CullMode::Front;
            desc.depth_bias = {
                .enable = true,
                .constant_factor = 2.0f,
                .slope_factor = 1.5f,
                .clamp = 0.0f,
            };
            return desc;
        }
    };
} // namespace enishi::types