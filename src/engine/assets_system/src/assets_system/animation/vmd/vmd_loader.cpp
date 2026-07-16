#include "vmd_loader.h"

namespace enishi::assets_system {
    VMDLoader::Result VMDLoader::load_vmd(BinaryReader& binary_reader, VMDData* const vmd_data) {
        // ヘッダ読み込み
        auto result = this->load_vmd_header(binary_reader, vmd_data);
        if (result.is_err()) {
            return result;
        }

        // ボーンキーフレームの読み込み
        result = this->load_vmd_bone_key_frame(binary_reader, vmd_data);
        if (result.is_err()) {
            return result;
        }

        // モーフの読み込み
        result = this->load_vmd_morph_key_frame(binary_reader, vmd_data);
        if (result.is_err()) {
            return result;
        }

        // カメラの読み込み
        result = this->load_vmd_camera(binary_reader, vmd_data);
        if (result.is_err()) {
            return result;
        }

        // ライトの読み込み
        result = this->load_vmd_light(binary_reader, vmd_data);
        if (result.is_err()) {
            return result;
        }

        // 影の読み込み
        result = this->load_vmd_shadow(binary_reader, vmd_data);
        if (result.is_err()) {
            return result;
        }

        // IKの読み込み
        result = this->load_vmd_ik(binary_reader, vmd_data);
        if (result.is_err()) {
            return result;
        }

        return {};
    }

    VMDLoader::Result VMDLoader::load_vmd_header(
        BinaryReader& binary_reader, VMDData* const vmd_data) {
        VMDHeader header{};
        auto result = binary_reader.read_to(&header);
        if (result.is_err()) {
            return result.unwrap_err().add_message("ヘッダを読み込めませんでした");
        }

        result = binary_reader.read_magic_number("Vocaloid Motion Data");
        if (result.is_ok()) {
            return {};
        }

        result = binary_reader.read_magic_number("Vocaloid Motion Data 0002");
        if (result.is_ok()) {
            return {};
        }

        return result;
    }

    VMDLoader::Result VMDLoader::load_vmd_bone_key_frame(
        BinaryReader& binary_reader, VMDData* const vmd_data) {
        std::uint32_t size; // サイズは4Byte
        auto result = binary_reader.read_to(&size);
        if (result.is_err()) {
            return result.unwrap_err().add_message(
                "ボーンキーフレームのサイズ読み込みに失敗しました");
        }

        result = binary_reader.read_to_vec(vmd_data->bone_key_frames, size);
        if (result.is_err()) {
            return result;
        }

        return {};
    }

    VMDLoader::Result VMDLoader::load_vmd_morph_key_frame(
        BinaryReader& binary_reader, VMDData* const vmd_data) {
        std::uint32_t size; // サイズは4Byte
        auto result = binary_reader.read_to(&size);
        if (result.is_err()) {
            return result.unwrap_err().add_message(
                "モーフキーフレームのサイズ読み込みに失敗しました");
        }

        result = binary_reader.read_to_vec(vmd_data->morph_key_frames, size);
        if (result.is_err()) {
            return result;
        }

        return {};
    }

    VMDLoader::Result VMDLoader::load_vmd_camera(
        BinaryReader& binary_reader, VMDData* const vmd_data) {
        std::uint32_t size; // サイズは4Byte

        auto result = binary_reader.read_to(&size);
        if (result.is_err()) {
            return result.unwrap_err().add_message("のサイズ読み込みに失敗しました");
        }

        result = binary_reader.read_to_vec(vmd_data->camera_key_frames, size);
        if (result.is_err()) {
            return result;
        }

        return {};
    }

    VMDLoader::Result VMDLoader::load_vmd_light(
        BinaryReader& binary_reader, VMDData* const vmd_data) {
        std::uint32_t size; // サイズは4Byte
        auto result = binary_reader.read_to(&size);
        if (result.is_err()) {
            return result.unwrap_err().add_message("のサイズ読み込みに失敗しました");
        }

        result = binary_reader.read_to_vec(vmd_data->light_key_frames, size);
        if (result.is_err()) {
            result.unwrap_err().add_message("");
            return result;
        }

        return {};
    }

    VMDLoader::Result VMDLoader::load_vmd_shadow(
        BinaryReader& binary_reader, VMDData* const vmd_data) {
        std::uint32_t size; // サイズは4Byte
        auto result = binary_reader.read_to(&size);
        if (result.is_err()) {
            return result.unwrap_err().add_message("のサイズ読み込みに失敗しました");
        }

        result = binary_reader.read_to_vec(vmd_data->shadow_key_frames, size);
        if (result.is_err()) {
            result.unwrap_err().add_message("");
            return result;
        }

        return {};
    }

    VMDLoader::Result VMDLoader::load_vmd_ik(BinaryReader& binary_reader, VMDData* const vmd_data) {
        std::uint32_t size; // サイズは4Byte

        auto result = binary_reader.read_to(&size);
        if (result.is_err()) {
            return result.unwrap_err().add_message("のサイズ読み込みに失敗しました");
        }

        vmd_data->iks.resize(size);
        for (auto& ik : vmd_data->iks) {
            //
            result =
                binary_reader.read(&ik, sizeof(VMDIKKeyFrame) - sizeof(VMDIKKeyFrame::ik_infos));
            if (result.is_err()) {
                return result.unwrap_err().add_message("");
            }

            result = binary_reader.read_to_vec(ik.ik_infos, ik.count);
            if (result.is_err()) {
                return result.unwrap_err().add_message("");
            }
        }

        return {};
    }

    IOResult<std::unique_ptr<VMDData>> VMDLoader::load(const std::filesystem::path& path) noexcept {
        auto reader = BinaryReader::make_reader(path);
        if (reader.is_err()) {
            return reader.unwrap_err();
        }
        auto& binary_reader = reader.unwrap_mut();
        VMDLoader loader{};
        std::unique_ptr<VMDData> vmd_data = std::make_unique<VMDData>();

        const auto result = loader.load_vmd(binary_reader, vmd_data.get());
        if (result.is_err()) {
            return result.unwrap_err();
        }

        return std::move(vmd_data);
    }
} // namespace enishi::assets_system