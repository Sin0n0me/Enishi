#include "texture_loader.h"
#include <nothings_std/stb_image.h>

namespace enishi::assets_system {
    foundation::Result<AssetData, AssetError> TextureLoader::load(
        const std::filesystem::path& path) noexcept {
        // TODO
        const bool force_srgb = false;

        std::int32_t width = 0;
        std::int32_t height = 0;
        std::int32_t channels = 0;
        stbi_uc* pixels =
            stbi_load(path.string<char>().c_str(), &width, &height, &channels, STBI_rgb_alpha);

        if (pixels == nullptr) {
            return foundation::Error(
                AssetError::IOError, std::format("stbi_load: {}", stbi_failure_reason()));
        };

        const types::TextureFormat format =
            force_srgb ? types::TextureFormat::RGBA8_SRGB : types::TextureFormat::RGBA8_UNORM;
        types::TextureData data{
            .format = format,
            .width = static_cast<std::uint32_t>(width),
            .height = static_cast<std::uint32_t>(height),
        };

        // ミップレベル0(元画像)
        const std::uint32_t row_pitch = data.width * 4;
        const std::uint32_t slice_pitch = row_pitch * data.height;

        types::MipData mip0{
            .width = data.width,
            .height = data.height,
            .row_pitch = row_pitch,
            .slice_pitch = slice_pitch,
        };

        mip0.pixels.assign(pixels, pixels + slice_pitch);
        data.mips.push_back(std::move(mip0));

        stbi_image_free(pixels);

        return data;
    }

    std::vector<foundation::UTF8> TextureLoader::get_supported_extension(void) const noexcept {
        return std::vector<foundation::UTF8>();
    }

    void TextureLoader::generate_mip_chain(types::TextureData& data) {
    }
} // namespace enishi::assets_system