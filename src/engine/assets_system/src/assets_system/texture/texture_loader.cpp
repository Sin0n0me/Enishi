#include "texture_loader.h"
#include <limits>

#define STB_IMAGE_IMPLEMENTATION
#include <nothings_std/stb_image.h>

namespace enishi::assets_system {
    foundation::Result<types::AssetTextureData, AssetError> TextureLoader::load_memory(
        std::span<const std::uint8_t> bytes) {
        if (bytes.empty() ||
            bytes.size() > static_cast<std::size_t>(std::numeric_limits<int>::max())) {
            return foundation::Error(AssetError::InvalidAssetData, "Invalid embedded image size");
        }
        int width{};
        int height{};
        int channels{};
        std::unique_ptr<stbi_uc, decltype(&stbi_image_free)> pixels(
            stbi_load_from_memory(bytes.data(),
                static_cast<int>(bytes.size()),
                &width,
                &height,
                &channels,
                STBI_rgb_alpha),
            stbi_image_free);
        if (pixels == nullptr || !(width > 0) || !(height > 0)) {
            const auto* reason = stbi_failure_reason();
            return foundation::Error(AssetError::InvalidAssetData,
                reason != nullptr ? reason : "Invalid embedded image");
        }
        constexpr std::uint32_t RGBA_CHANNELS = 4;
        const auto pixel_count = static_cast<std::uint64_t>(width) * height;
        if (pixel_count > std::numeric_limits<std::uint32_t>::max() / RGBA_CHANNELS) {
            return foundation::Error(AssetError::InvalidAssetData, "Embedded image is too large");
        }
        auto data = std::make_shared<types::TextureData>();
        data->format = types::TextureFormat::RGBA8_UNORM;
        data->width = static_cast<std::uint32_t>(width);
        data->height = static_cast<std::uint32_t>(height);
        data->array_size = 1;
        types::MipData mip;
        mip.width = data->width;
        mip.height = data->height;
        // RGBA8 row bytes = width * 4; total bytes = row bytes * height.
        mip.row_pitch = data->width * RGBA_CHANNELS;
        mip.slice_pitch = mip.row_pitch * data->height;
        mip.pixels.assign(pixels.get(), pixels.get() + mip.slice_pitch);
        data->mips.push_back(std::move(mip));
        return data;
    }

    foundation::Result<types::AssetData, AssetError> TextureLoader::load(
        const std::filesystem::path& path) noexcept {
        // キャッシュがあれば使用
        const auto iter = this->cache.find(path.lexically_normal());
        if (iter != this->cache.end()) {
            return types::AssetData{iter->second};
        }

        // TODO
        const bool force_srgb = false;

        // 常に RGBA8 に変換する(GPU側で統一フォーマットを扱うため)
        std::int32_t width = 0;
        std::int32_t height = 0;
        std::int32_t channels = 0;
        stbi_uc* const pixels =
            stbi_load(path.string<char>().c_str(), &width, &height, &channels, STBI_rgb_alpha);

        if (pixels == nullptr) {
            const char* const reason = stbi_failure_reason();
            return foundation::Error(AssetError::IOError,
                std::format("Failed to load texture {}: {}",
                    path.string(),
                    reason != nullptr ? reason : "Unknown image decoding error"));
        };

        const types::TextureFormat format =
            force_srgb ? types::TextureFormat::RGBA8_SRGB : types::TextureFormat::RGBA8_UNORM;
        const auto data = std::make_shared<types::TextureData>();
        data->format = format;
        data->width = static_cast<std::uint32_t>(width);
        data->height = static_cast<std::uint32_t>(height);
        data->array_size = 1;

        // ミップレベル0(元画像)
        const std::uint32_t row_pitch = data->width * 4;
        const std::uint32_t slice_pitch = row_pitch * data->height;
        types::MipData mip0;
        mip0.width = data->width;
        mip0.height = data->height;
        mip0.row_pitch = row_pitch;
        mip0.slice_pitch = slice_pitch;
        mip0.pixels.assign(pixels, pixels + slice_pitch);
        data->mips.push_back(std::move(mip0));

        stbi_image_free(pixels);

        return types::AssetData{data};
    }

    std::vector<foundation::UTF8> TextureLoader::get_supported_extension(void) const noexcept {
        return {
            ".png",
            ".jpg",
            ".jpeg",
            ".jpe",
            ".bmp",
            ".tga",
            ".tpic",
            ".gif",
            ".psd",
            ".hdr",
            ".pic",
            ".ppm",
            ".pgm",
            ".spa",
            ".sph",
        };
    }

    void TextureLoader::generate_mip_chain(types::TextureData& data) {
        std::uint32_t mip_width = data.width;
        std::uint32_t mip_height = data.height;

        while (mip_width > 1 || mip_height > 1) {
            const std::uint32_t prev_width = mip_width;
            const std::uint32_t prev_height = mip_height;
            mip_width = std::max(1u, mip_width / 2);
            mip_height = std::max(1u, mip_height / 2);

            const auto& prev_mip = data.mips.back();
            const auto slice_pitch = mip_width * 4 * mip_height;
            types::MipData mip;
            mip.pixels = decltype(types::MipData::pixels)(slice_pitch);
            mip.width = mip_width;
            mip.height = mip_height;
            mip.row_pitch = mip_width * 4;
            mip.slice_pitch = slice_pitch;

            // 2×2ボックスフィルタで縮小
            for (std::uint32_t y = 0; y < mip_height; ++y) {
                for (std::uint32_t x = 0; x < mip_width; ++x) {
                    for (std::uint32_t c = 0; c < 4; ++c) {
                        // 対応する2×2ピクセルを平均
                        const auto sx = x * 2;
                        const auto sy = y * 2;
                        const auto min_height = std::min(sy + 1, prev_height - 1);
                        const auto min_width = std::min(sx + 1, prev_width - 1);
                        const auto w = sy * prev_width;
                        const auto h = min_height * prev_width;

                        const auto p00_index = (w + sx) * 4 + c;
                        const auto p01_index = (h + sx) * 4 + c;
                        const auto p10_index = (w + min_width) * 4 + c;
                        const auto p11_index = (h + min_width) * 4 + c;
                        const auto p00 = prev_mip.pixels[p00_index];
                        const auto p01 = prev_mip.pixels[p01_index];
                        const auto p10 = prev_mip.pixels[p10_index];
                        const auto p11 = prev_mip.pixels[p11_index];

                        const auto index = (y * mip_width + x) * 4 + c;
                        mip.pixels[index] = static_cast<std::uint8_t>((p00 + p01 + p10 + p11) / 4);
                    }
                }
            }

            data.mips.emplace_back(std::move(mip));
        }
    }

    types::AssetKind TextureLoader::get_target_asset_type(void) const noexcept {
        return types::AssetKind::Texture;
    }
} // namespace enishi::assets_system
