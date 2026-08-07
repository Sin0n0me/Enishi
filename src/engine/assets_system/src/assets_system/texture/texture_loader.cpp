#include "texture_loader.h"

#define STB_IMAGE_IMPLEMENTATION
#include <nothings_std/stb_image.h>

namespace enishi::assets_system {
    foundation::Result<AssetData, AssetError> TextureLoader::load(
        const std::filesystem::path& path) noexcept {
        // TODO
        const bool force_srgb = false;

        // 常に RGBA8 に変換する(GPU側で統一フォーマットを扱うため)
        std::int32_t width = 0;
        std::int32_t height = 0;
        std::int32_t channels = 0;
        stbi_uc* const pixels =
            stbi_load(path.string<char>().c_str(), &width, &height, &channels, STBI_rgb_alpha);

        if (pixels == nullptr) {
            return foundation::Error(
                AssetError::IOError, std::format("stbi_load: {}", stbi_failure_reason()));
        };

        const types::TextureFormat format =
            force_srgb ? types::TextureFormat::RGBA8_SRGB : types::TextureFormat::RGBA8_UNORM;
        const auto data = std::make_shared<types::TextureData>();
        data->format = format;
        data->width = static_cast<std::uint32_t>(width);
        data->height = static_cast<std::uint32_t>(height);

        // ミップレベル0(元画像)
        const std::uint32_t row_pitch = data->width * 4;
        const std::uint32_t slice_pitch = row_pitch * data->height;
        types::MipData mip0{
            .width = data->width,
            .height = data->height,
            .row_pitch = row_pitch,
            .slice_pitch = slice_pitch,
        };
        mip0.pixels.assign(pixels, pixels + slice_pitch);
        data->mips.push_back(std::move(mip0));

        stbi_image_free(pixels);

        return AssetData{data};
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
            types::MipData mip{
                .pixels = decltype(types::MipData::pixels)(mip.slice_pitch),
                .width = mip_width,
                .height = mip_height,
                .row_pitch = mip_width * 4,
                .slice_pitch = mip.row_pitch * mip_height,
            };

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

    AssetType TextureLoader::get_target_asset_type(void) const noexcept {
        return AssetType::Texture;
    }
} // namespace enishi::assets_system