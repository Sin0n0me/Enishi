#pragma once
#include <cstdint>

namespace enishi::types {
    enum class ImageViewType : std::uint8_t {
        Unknown,
        RenderTarget,   // DirectXでいうところのRTV カラーバッファへの描画
        DepthStencil,   // DirectXでいうところのDSV 深度・ステンシル
        ShaderResource, // DirectXでいうところのSRV シェーダからの読み取り
        UnorderedAccess // DirectXでいうところのUAV コンピュートの読み書き
    };

    enum class ImageFormat : std::uint8_t {
        // カラー
        RGBA8_UNORM,
        RGBA16_FLOAT,
        BGRA8_UNORM, // スワップチェーン用

        // 深度
        D32_FLOAT,
        D24_UNORM_S8_UINT,
        D16_UNORM,

        // 圧縮テクスチャ
        BC7_UNORM,
        BC3_UNORM,
    };

    enum class ImageAspect : std::uint8_t {
        Color,
        Depth,
        Stencil,
        DepthStencil,
    };

    struct ImageViewDescription {
        ImageViewType type;
        ImageFormat format;
        ImageAspect aspect;
        std::uint32_t width;
        std::uint32_t height;
        std::uint32_t base_mip_level;
        std::uint32_t mip_Level_count;
        std::uint32_t base_array_layer;
        std::uint32_t array_layer_count;

        // RTV用のデフォルト記述子を生成するヘルパー
        [[nodiscard]]
        static constexpr ImageViewDescription make_render_target_view_description(
            const ImageFormat format) noexcept {
            return ImageViewDescription{
                .type = ImageViewType::RenderTarget,
                .format = format,
                .aspect = ImageAspect::Color,
                .base_mip_level = 0,
                .mip_Level_count = 1,
                .base_array_layer = 0,
                .array_layer_count = 1,
            };
        }

        // DSV用
        [[nodiscard]]
        static constexpr ImageViewDescription make_depth_stencil_view_description(
            const ImageFormat format) noexcept {
            return ImageViewDescription{
                .type = ImageViewType::DepthStencil,
                .format = format,
                .aspect = ImageAspect::DepthStencil,
                .base_mip_level = 0,
                .mip_Level_count = 1,
                .base_array_layer = 0,
                .array_layer_count = 1,
            };
        }

        // SRV用（ミップマップあり）
        [[nodiscard]]
        static constexpr ImageViewDescription make_shader_resource_view_description(
            const ImageFormat format, const std::uint32_t mip_level = 1) noexcept {
            return ImageViewDescription{
                .type = ImageViewType::ShaderResource,
                .format = format,
                .aspect = ImageAspect::Color,
                .base_mip_level = mip_level,
                .mip_Level_count = 1,
                .base_array_layer = 0,
                .array_layer_count = 1,
            };
        }

        // UAV用
        [[nodiscard]]
        static constexpr ImageViewDescription make_unordered_access_view_description(
            ImageFormat format, uint32_t mip_level = 0) noexcept {
            return ImageViewDescription{
                .type = ImageViewType::UnorderedAccess,
                .format = format,
                .aspect = ImageAspect::Color,
                .base_mip_level = mip_level,
                .mip_Level_count = 1,
                .base_array_layer = 0,
                .array_layer_count = 1,
            };
        }
    };
} // namespace enishi::types