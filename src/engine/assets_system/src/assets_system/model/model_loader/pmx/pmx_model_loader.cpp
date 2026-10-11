#include "pmx_model_loader.h"
#include "../../../utility/binary_reader.h"
#include "pmx_reader.h"

namespace enishi::assets_system {
    foundation::Result<PMXData, AssetError> PMXModelLoader::parse(
        std::span<const std::uint8_t> bytes) {
        return parse_pmx(bytes);
    }

    foundation::Result<ModelVariant, AssetError> PMXModelLoader::load(
        const std::filesystem::path& path) noexcept {
        auto reader = BinaryReader::make_reader(path);
        if (reader.is_err()) {
            return reader.propagation(AssetError::IOError);
        }
        auto bytes = reader.unwrap_mut().read_all();
        if (bytes.is_err()) {
            return bytes.propagation(AssetError::IOError);
        }
        auto data = PMXModelLoader::parse(bytes.unwrap());
        if (data.is_err()) {
            return std::move(data).take_err();
        }
        return ModelVariant{std::make_unique<PMXData>(std::move(data).unwrap_mut())};
    }

    foundation::UTF8 PMXModelLoader::get_supported_extension(void) const noexcept {
        return ".pmx";
    }
} // namespace enishi::assets_system
