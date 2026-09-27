#include "vmd_loader.h"
#include <algorithm>
#include <array>

namespace enishi::assets_system {
    VMDLoader::Result VMDLoader::load_vmd(BinaryReader& reader, VMDData* const data) {
        auto result = this->load_vmd_header(reader, data);
        if (result.is_err()) { return result; }
        result = this->load_vmd_bone_key_frame(reader, data);
        if (result.is_err()) { return result; }
        const std::array optional_sections{&VMDLoader::load_vmd_morph_key_frame,
            &VMDLoader::load_vmd_camera, &VMDLoader::load_vmd_light,
            &VMDLoader::load_vmd_shadow, &VMDLoader::load_vmd_ik};
        for (const auto section : optional_sections) {
            const auto remaining = reader.remaining_bytes();
            if (remaining.is_err()) {
                return remaining.propagation(IOError::ReadFailed);
            }
            if (remaining.unwrap() == 0) { return {}; }
            result = (this->*section)(reader, data);
            if (result.is_err()) { return result; }
        }
        return {};
    }

    VMDLoader::Result VMDLoader::load_vmd_header(
        BinaryReader& binary_reader, VMDData* const vmd_data) {
        VMDHeader header{};

        {
            auto&& result =
                binary_reader.read_to(&header).add_message("ヘッダを読み込めませんでした");
            if (result.is_err()) {
                return result;
            }
        }

        const std::string signature(std::begin(header.header),
            std::find(std::begin(header.header), std::end(header.header), '\0'));
        if (signature != "Vocaloid Motion Data 0002" && signature != "Vocaloid Motion Data") {
            return foundation::Error(IOError::MismatchHeader, "Invalid VMD signature");
        }
        std::copy(std::begin(header.model_name), std::end(header.model_name), vmd_data->name.begin());
        return {};
    }

    VMDLoader::Result VMDLoader::load_vmd_bone_key_frame(
        BinaryReader& binary_reader, VMDData* const vmd_data) {
        std::uint32_t size; // サイズは4Byte
        {
            auto&& result = binary_reader.read_to(&size).add_message(
                "ボーンキーフレームのサイズ読み込みに失敗しました");
            if (result.is_err()) {
                return result;
            }
        }

        {
            auto&& result = binary_reader.read_to_vec(vmd_data->bone_key_frames, size);
            if (result.is_err()) {
                return result;
            }
        }

        return {};
    }

    VMDLoader::Result VMDLoader::load_vmd_morph_key_frame(
        BinaryReader& binary_reader, VMDData* const vmd_data) {
        std::uint32_t size; // サイズは4Byte
        {
            auto&& result = binary_reader.read_to(&size).add_message(
                "モーフキーフレームのサイズ読み込みに失敗しました");
            if (result.is_err()) {
                return result;
            }
        }

        {
            auto&& result = binary_reader.read_to_vec(vmd_data->morph_key_frames, size);
            if (result.is_err()) {
                return result;
            }
        }

        return {};
    }

    VMDLoader::Result VMDLoader::load_vmd_camera(
        BinaryReader& binary_reader, VMDData* const vmd_data) {
        std::uint32_t size; // サイズは4Byte
        {
            auto&& result =
                binary_reader.read_to(&size).add_message("のサイズ読み込みに失敗しました");
            if (result.is_err()) {
                return result;
            }
        }

        {
            auto&& result = binary_reader.read_to_vec(vmd_data->camera_key_frames, size);
            if (result.is_err()) {
                return result;
            }
        }

        return {};
    }

    VMDLoader::Result VMDLoader::load_vmd_light(
        BinaryReader& binary_reader, VMDData* const vmd_data) {
        std::uint32_t size; // サイズは4Byte
        {
            auto&& result =
                binary_reader.read_to(&size).add_message("のサイズ読み込みに失敗しました");
            if (result.is_err()) {
                return result;
            }
        }

        {
            auto&& result =
                binary_reader.read_to_vec(vmd_data->light_key_frames, size).add_message("");
            if (result.is_err()) {
                return result;
            }
        }

        return {};
    }

    VMDLoader::Result VMDLoader::load_vmd_shadow(
        BinaryReader& binary_reader, VMDData* const vmd_data) {
        std::uint32_t size; // サイズは4Byte
        {
            auto&& result =
                binary_reader.read_to(&size).add_message("のサイズ読み込みに失敗しました");
            if (result.is_err()) {
                return result;
            }
        }

        {
            auto&& result =
                binary_reader.read_to_vec(vmd_data->shadow_key_frames, size).add_message("");
            if (result.is_err()) {
                return result;
            }
        }

        return {};
    }

    VMDLoader::Result VMDLoader::load_vmd_ik(BinaryReader& binary_reader, VMDData* const vmd_data) {
        std::uint32_t size; // サイズは4Byte

        {
            auto&& result =
                binary_reader.read_to(&size).add_message("のサイズ読み込みに失敗しました");
            if (result.is_err()) {
                return result;
            }
        }

        constexpr std::size_t IK_HEADER_BYTES = sizeof(std::uint32_t) * 2 + sizeof(std::uint8_t);
        const auto remaining = binary_reader.remaining_bytes();
        if (remaining.is_err()) {
            return remaining.propagation(IOError::ReadFailed);
        }
        if (size > remaining.unwrap() / IK_HEADER_BYTES) {
            return foundation::Error(IOError::UnexpectedEof, "IK frame count exceeds file size");
        }
        vmd_data->iks.resize(size);
        for (auto& ik : vmd_data->iks) {
            auto result = binary_reader.read_to(&ik.frame);
            if (result.is_err()) { return result; }
            result = binary_reader.read_to(&ik.show_flag);
            if (result.is_err()) { return result; }
            result = binary_reader.read_to(&ik.count);
            if (result.is_err()) { return result; }
            result = binary_reader.read_to_vec(ik.ik_infos, ik.count);
            if (result.is_err()) { return result; }
        }

        return {};
    }

    IOResult<std::unique_ptr<VMDData>> VMDLoader::load(const std::filesystem::path& path) noexcept {
        auto reader = BinaryReader::make_reader(path);
        if (reader.is_err()) {
            return std::move(reader).take_err();
        }
        auto& binary_reader = reader.unwrap_mut();
        VMDLoader loader{};
        std::unique_ptr<VMDData> vmd_data = std::make_unique<VMDData>();

        auto&& result = loader.load_vmd(binary_reader, vmd_data.get());
        if (result.is_err()) {
            return std::move(result).take_err();
        }

        return vmd_data;
    }
} // namespace enishi::assets_system