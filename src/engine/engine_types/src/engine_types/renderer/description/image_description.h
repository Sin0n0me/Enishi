#pragma once
#include "image_view_description.h"
#include <cstdint>
#include <glm/glm.hpp>

namespace enishi::types {
    enum class ImageUsage : std::uint32_t {
        None = 0,
        RenderTarget = 1 << 0,    // RTV として使う
        DepthStencil = 1 << 1,    // DSV として使う
        ShaderResource = 1 << 2,  // SRV として使う
        UnorderedAccess = 1 << 3, // UAV として使う
        TransferSrc = 1 << 4,     // コピー元
        TransferDst = 1 << 5,     // コピー先
        BackBuffer = 1 << 6,      // バックバッファから
    };

    // ビット演算を使えるようにする
    [[nodiscard]]
    constexpr ImageUsage operator|(const ImageUsage& a, const ImageUsage& b) noexcept {
        return static_cast<ImageUsage>(
            static_cast<std::uint32_t>(a) | static_cast<std::uint32_t>(b));
    }

    [[nodiscard]]
    constexpr ImageUsage operator&(const ImageUsage& a, const ImageUsage& b) noexcept {
        return static_cast<ImageUsage>(
            static_cast<std::uint32_t>(a) & static_cast<std::uint32_t>(b));
    }

    [[nodiscard]]
    constexpr ImageUsage operator^(const ImageUsage& a, const ImageUsage& b) noexcept {
        return static_cast<ImageUsage>(
            static_cast<std::uint32_t>(a) ^ static_cast<std::uint32_t>(b));
    }

    [[nodiscard]]
    constexpr bool has_usage(const ImageUsage& flags, const ImageUsage& bit) noexcept {
        return (static_cast<std::uint32_t>(flags) & static_cast<std::uint32_t>(bit)) != 0;
    }

    struct ImageDescription {
        glm::ivec2 size;
        ImageFormat format;
        ImageUsage usage;
        std::uint32_t mip_levels;
        std::uint32_t array_layers;
        std::uint32_t samples; // MSAA サンプル数

        [[nodiscard]]
        bool contains(const ImageUsage flag) const noexcept {
            return has_usage(this->usage, flag);
        }

        [[nodiscard]]
        static constexpr ImageDescription make_default(
            const glm::ivec2& size, const ImageFormat& format, const ImageUsage& usage) noexcept {
            return ImageDescription{
                .size = size,
                .format = format,
                .usage = usage,
                .mip_levels = 1,
                .array_layers = 1,
                .samples = 1,
            };
        }

        // スワップチェイン側に書く場合
        [[nodiscard]]
        static constexpr ImageDescription make_default_render_target(
            const glm::ivec2& size, const ImageFormat format = ImageFormat::RGBA8_UNORM) noexcept {
            return ImageDescription::make_default(size,
                format,
                ImageUsage::RenderTarget | ImageUsage::ShaderResource | ImageUsage::BackBuffer);
        }

        [[nodiscard]]
        static constexpr ImageDescription make_render_target(
            const glm::ivec2& size, const ImageFormat format = ImageFormat::RGBA8_UNORM) noexcept {
            return ImageDescription::make_default(
                size, format, ImageUsage::RenderTarget | ImageUsage::ShaderResource);
        }

        [[nodiscard]]
        static constexpr ImageDescription make_depth_stencil(
            const glm::ivec2& size, const ImageFormat format = ImageFormat::D32_FLOAT) noexcept {
            return ImageDescription::make_default(size, format, ImageUsage::DepthStencil);
        }

        [[nodiscard]]
        static constexpr ImageDescription make_texture(const glm::ivec2& size,
            const ImageFormat format = ImageFormat::RGBA8_UNORM,
            const std::uint32_t mip_levels = 1) noexcept {
            auto description =
                ImageDescription::make_default(size, format, ImageUsage::ShaderResource);
            description.mip_levels = mip_levels;

            return description;
        }
    };
} // namespace enishi::types