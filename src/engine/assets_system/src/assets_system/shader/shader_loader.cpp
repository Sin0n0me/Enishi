#include "shader_loader.h"

namespace enishi::assets_system {
    constexpr std::uint32_t SPIR_V_HEADER = 0x07230203;
    constexpr char DXBC_HEADER[] = "DXBC";

    ShaderLoader::ShaderLoader(void)
        : supported_extension({
              {".hlsl", types::ShaderBinaryType::SourceFileHLSL},
              {".glsl", types::ShaderBinaryType::SourceFileGLSL},
              {".metal", types::ShaderBinaryType::SourceFileMSL},
              {".spv", types::ShaderBinaryType::SPIR_V},
              {".dxil", types::ShaderBinaryType::DXIL},
              {".dxbc", types::ShaderBinaryType::DXBC},
          }) {
    }

    foundation::Result<AssetData, AssetError> ShaderLoader::load(
        const std::filesystem::path& path) noexcept {
        auto reader = BinaryReader::make_reader(path);
        if (reader.is_err()) {
            return reader.propagation(AssetError::IOError);
        }
        auto& binary_reader = reader.unwrap_mut();

        if (!path.has_extension()) {
            return foundation::Error(AssetError::NotFound);
        }

        const auto iter = this->supported_extension.find(path.extension().string<char>().c_str());
        if (iter == this->supported_extension.end()) {
            return foundation::Error(AssetError::NotFound);
        }
        switch (iter->second) {
            case types::ShaderBinaryType::SPIR_V:
                return ShaderLoader::load_spir_v(binary_reader);
            case types::ShaderBinaryType::DXBC:
                return ShaderLoader::load_dxbc(binary_reader);
            case types::ShaderBinaryType::DXIL:
            case types::ShaderBinaryType::SourceFileGLSL:
            case types::ShaderBinaryType::SourceFileHLSL:
            case types::ShaderBinaryType::SourceFileMSL:
            default:
                break;
        }

        return foundation::Error(AssetError::NotFound);
    }

    std::vector<foundation::UTF8> ShaderLoader::get_supported_extension(void) const noexcept {
        std::vector<foundation::UTF8> extensions;
        extensions.reserve(this->supported_extension.size());
        for (const auto& [extension, _] : this->supported_extension) {
            extensions.push_back(extension);
        }

        return extensions;
    }

    foundation::Result<types::ShaderData, AssetError> ShaderLoader::load_spir_v(
        BinaryReader& reader) noexcept {
        auto result = reader.read_all();
        if (result.is_err()) {
            return result.propagation(AssetError::IOError);
        }

        if (reader.read_magic_number(SPIR_V_HEADER).is_err()) {
            return foundation::Error(AssetError::InvalidAssetData);
        }

        return types::ShaderData{
            .binary_type = types::ShaderBinaryType::SPIR_V,
            .code = std::move(result.unwrap_mut()),
        };
    }

    foundation::Result<types::ShaderData, AssetError> ShaderLoader::load_dxbc(
        BinaryReader& reader) noexcept {
        auto result = reader.read_all();
        if (result.is_err()) {
            return result.propagation(AssetError::IOError);
        }

        if (reader.read_magic_number_from_str(DXBC_HEADER).is_err()) {
            return foundation::Error(AssetError::InvalidAssetData);
        }

        return types::ShaderData{
            .binary_type = types::ShaderBinaryType::DXBC,
            .code = std::move(result.unwrap_mut()),
        };
    }

    AssetType ShaderLoader::get_target_asset_type(void) const noexcept {
        return AssetType::Shader;
    }
} // namespace enishi::assets_system