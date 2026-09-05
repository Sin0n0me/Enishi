#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace enishi::types {
    enum class TextureFormat : std::uint8_t {
        // 非圧縮
        R8_UNORM,
        RG8_UNORM,
        RGBA8_UNORM,
        RGBA8_SRGB,
        RGBA16_FLOAT,

        // GPU圧縮（BC / DXT）
        BC1_UNORM, // DXT1: RGB 不透明・1bitアルファ
        BC3_UNORM, // DXT5: RGBA
        BC4_UNORM, // R1チャンネル（ハイトマップなど）
        BC5_UNORM, // RG2チャンネル（法線マップ）
        BC7_UNORM, // 高品質RGBA
        BC7_SRGB,
    };

    // 1ミップレベル分のデータ
    struct MipData {
        std::vector<std::uint8_t> pixels;
        std::uint32_t width;
        std::uint32_t height;
        std::uint32_t row_pitch;   // 1行のバイト数
        std::uint32_t slice_pitch; // 全体のバイト数

        MipData(void) = default;
        MipData(MipData&&) = default;
        MipData& operator=(MipData&&) = default;
    };

    struct TextureData {
        TextureFormat format;
        bool is_cubemap;
        std::uint32_t width;
        std::uint32_t height;
        std::uint32_t depth;       // 3Dテクスチャ用
        std::uint32_t array_size;  // テクスチャ配列用
        std::vector<MipData> mips; // mips[0]が最大解像度

        TextureData(void) = default;
        TextureData(TextureData&&) = default;
        TextureData& operator=(TextureData&&) = default;

        // BC圧縮用の format のブロックサイズを返す
        [[nodiscard]]
        static constexpr std::uint32_t block_size(const TextureFormat& fmt) noexcept {
            switch (fmt) {
                case TextureFormat::BC1_UNORM:
                    return 8;
                case TextureFormat::BC3_UNORM:
                    return 16;
                case TextureFormat::BC4_UNORM:
                    return 8;
                case TextureFormat::BC5_UNORM:
                    return 16;
                case TextureFormat::BC7_UNORM:
                    return 16;
                case TextureFormat::BC7_SRGB:
                    return 16;
                default:
                    return 0; // 非圧縮
            }
        }

        // format が BC圧縮かどうか
        [[nodiscard]]
        static constexpr bool is_compressed(const TextureFormat& fmt) noexcept {
            return block_size(fmt) > 0;
        }

        // 非圧縮フォーマットの1ピクセルあたりのバイト数
        [[nodiscard]]
        static constexpr std::uint32_t bytes_per_pixel(const TextureFormat& fmt) noexcept {
            switch (fmt) {
                case TextureFormat::R8_UNORM:
                    return 1;
                case TextureFormat::RG8_UNORM:
                    return 2;
                case TextureFormat::RGBA8_UNORM:
                    return 4;
                case TextureFormat::RGBA8_SRGB:
                    return 4;
                case TextureFormat::RGBA16_FLOAT:
                    return 8;
                default:
                    return 0;
            }
        }
    };
} // namespace enishi::types