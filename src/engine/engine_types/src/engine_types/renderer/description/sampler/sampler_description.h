#pragma once
#include "detail/sampler_mode.h"
#include <cstdint>

namespace enishi::types {
    struct SamplerStateDescription {
        FilterMode mag_filter;
        FilterMode min_filter;
        FilterMode mip_filter;
        AddressMode address_u;
        AddressMode address_v;
        AddressMode address_w;
        AnisotropyLevel anisotropy;
        float mip_lod_bias;
        float min_lod;
        float max_lod;

        [[nodiscard]]
        static constexpr SamplerStateDescription default_linear(void) noexcept {
            return SamplerStateDescription{
                .mag_filter = FilterMode::Linear,
                .min_filter = FilterMode::Linear,
                .mip_filter = FilterMode::Linear,
                .address_u = AddressMode::Repeat,
                .address_v = AddressMode::Repeat,
                .address_w = AddressMode::Repeat,
                .anisotropy = AnisotropyLevel::X4,
                .mip_lod_bias = 0.0f,
                .min_lod = 0.0f,
                .max_lod = 1000.0f,
            };
        }

        [[nodiscard]]
        static constexpr SamplerStateDescription default_nearest(void) noexcept {
            return SamplerStateDescription{
                .mag_filter = FilterMode::Nearest,
                .min_filter = FilterMode::Nearest,
                .mip_filter = FilterMode::Nearest,
                .address_u = AddressMode::Repeat,
                .address_v = AddressMode::Repeat,
                .address_w = AddressMode::Repeat,
                .anisotropy = AnisotropyLevel::None,
                .mip_lod_bias = 0.0f,
                .min_lod = 0.0f,
                .max_lod = 1000.0f,
            };
        }

        [[nodiscard]]
        static SamplerStateDescription clamp_linear(void) noexcept {
            return SamplerStateDescription{
                .mag_filter = FilterMode::Linear,
                .min_filter = FilterMode::Linear,
                .mip_filter = FilterMode::Linear,
                .address_u = AddressMode::Clamp,
                .address_v = AddressMode::Clamp,
                .address_w = AddressMode::Clamp,
                .anisotropy = AnisotropyLevel::X4,
                .mip_lod_bias = 0.0f,
                .min_lod = 0.0f,
                .max_lod = 1000.0f,
            };
        }
    };
} // namespace enishi::types