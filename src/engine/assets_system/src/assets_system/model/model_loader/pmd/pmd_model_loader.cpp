#include "pmd_model_loader.h"

namespace enishi::assets_system {
    // 先頭から順に
    // ヘッダー(284Byte)
    // 頂点数(4Byte)
    // 頂点データ(40Byte * 頂点数)
    // インデックス数(4Byte)
    // インデックスデータ(2Byte * インデックス数)
    // マテリアル数(4Byte)
    // マテリアルデータ(70Byte)
    // ボーン数(2Byte)
    // ボーンデータ
    // IK数
    // IKデータ
    PMDModelLoader::Result PMDModelLoader::load_pmd(
        BinaryReader& binary_reader, PMDData* const pmd_data) {
        // モデルデータの読み込み開始

        // ヘッダ読み込み
        auto result = this->load_pmd_header(binary_reader, pmd_data);
        if (result.is_err()) {
            return result;
        }

        // vertices
        result = this->load_pmd_vertices(binary_reader, pmd_data);
        if (result.is_err()) {
            return result;
        }

        // indices
        result = this->load_pmd_indices(binary_reader, pmd_data);
        if (result.is_err()) {
            return result;
        }

        // materials
        result = this->load_pmd_materials(binary_reader, pmd_data);
        if (result.is_err()) {
            return result;
        }

        // bone_matrices
        result = this->load_pmd_bones(binary_reader, pmd_data);
        if (result.is_err()) {
            return result;
        }

        // IK
        result = this->load_pmd_iks(binary_reader, pmd_data);
        if (result.is_err()) {
            return result;
        }

        // モーフ(表情)
        result = this->load_pmd_morphs(binary_reader, pmd_data);
        if (result.is_err()) {
            return result;
        }

        // モーフの表示リスト
        result = this->load_pmd_display_morphs(binary_reader, pmd_data);
        if (result.is_err()) {
            return result;
        }

        // ボーンフレーム
        result = this->load_pmd_bone_frame_names(binary_reader, pmd_data);
        if (result.is_err()) {
            return result;
        }

        // 表示ボーン
        result = this->load_pmd_display_bones(binary_reader, pmd_data);
        if (result.is_err()) {
            return result;
        }

        // 以下拡張部分

        // 英名対応
        result = this->load_pmd_english_dictionary(binary_reader, pmd_data);
        if (result.is_err()) {
            return result;
        }

        // トゥーンテクスチャリスト
        result = this->load_pmd_toon_textures(binary_reader, pmd_data);
        if (result.is_err()) {
            return result;
        }

        // 物理剛体
        result = this->load_pmd_rigid_bodies(binary_reader, pmd_data);
        if (result.is_err()) {
            return result;
        }

        // ジョイント
        result = this->load_pmd_physics_joints(binary_reader, pmd_data);
        if (result.is_err()) {
            return result;
        }

        return {};
    }

    PMDModelLoader::Result PMDModelLoader::load_pmd_header(
        BinaryReader& binary_reader, PMDData* const pmd_data) {
        PMDHeader header{};

        auto result = binary_reader.read_to(&header);
        if (result.is_err()) {
            return result.add_message("ヘッダを読み込めませんでした");
        }

        result = binary_reader.read_magic_number_from_str("Pmd");
        if (result.is_err()) {
            return result;
        }

        return {};
    }

    PMDModelLoader::Result PMDModelLoader::load_pmd_vertices(
        BinaryReader& binary_reader, PMDData* const pmd_data) {
        std::uint32_t size; // サイズは4Byte

        {
            auto result = binary_reader.read_to(&size);
            if (result.is_err()) {
                return result.add_message("頂点情報リストサイズの取得に失敗しました");
            }
        }

        {
            auto result = binary_reader.read_to_vec(pmd_data->vertices, size);
            if (result.is_err()) {
                return result.add_message("頂点情報の取得に失敗しました");
            }
        }

        // Logger::info("頂点情報の読み込みに成功しました");
        return {};
    }

    PMDModelLoader::Result PMDModelLoader::load_pmd_indices(
        BinaryReader& binary_reader, PMDData* const pmd_data) {
        std::uint32_t size; // サイズは4Byte

        {
            auto result = binary_reader.read_to(&size);
            if (result.is_err()) {
                return result.add_message("インデックスリストサイズの取得に失敗しました");
            }
        }

        {
            auto result = binary_reader.read_to_vec(pmd_data->indices, size);
            if (result.is_err()) {
                return result.add_message("インデックス情報の取得に失敗しました");
            }
        }

        // Logger::info("インデックス情報の読み込みに成功しました");
        return {};
    }

    PMDModelLoader::Result PMDModelLoader::load_pmd_materials(
        BinaryReader& binary_reader, PMDData* const pmd_data) {
        std::uint32_t size; // サイズは4Byte

        {
            auto result = binary_reader.read_to(&size);
            if (result.is_err()) {
                return result.add_message("マテリアルリストサイズの取得に失敗しました");
            }
        }

        {
            auto result = binary_reader.read_to_vec(pmd_data->materials, size);
            if (result.is_err()) {
                return result.add_message("マテリアル情報の取得に失敗しました");
            }
        }

        // Logger::info("マテリアル情報の読み込みに成功しました");
        return {};
    }

    PMDModelLoader::Result PMDModelLoader::load_pmd_bones(
        BinaryReader& binary_reader, PMDData* const pmd_data) {
        std::uint16_t size; // サイズは2Byte

        {
            auto result = binary_reader.read_to(&size);
            if (result.is_err()) {
                return result.add_message("ボーンリストサイズの取得に失敗しました");
            }
        }

        {
            auto result = binary_reader.read_to_vec(pmd_data->bones, size);
            if (result.is_err()) {
                return result.add_message("ボーン情報の取得に失敗しました");
            }
        }

        // Logger::info("ボーン情報の読み込みに成功しました");
        return {};
    }

    PMDModelLoader::Result PMDModelLoader::load_pmd_iks(
        BinaryReader& binary_reader, PMDData* const pmd_data) {
        std::uint16_t size; // サイズは2Byte

        {
            auto result = binary_reader.read_to(&size);
            if (result.is_err()) {
                return result.add_message("IKリストサイズの取得に失敗しました");
            }
        }

        pmd_data->iks.resize(size);
        for (auto& ik : pmd_data->iks) {
            // 可変部分であるchainの部分だけは後で読み込むように
            // 先頭11Byteだけ読み込み
            {
                auto result = binary_reader.read(&ik, sizeof(PMDIK) - sizeof(PMDIK::chain));
                if (result.is_err()) {
                    return result.add_message("IK情報の取得に失敗しました");
                }
            }

            // 可変部分
            {
                auto result = binary_reader.read_to_vec(ik.chain, ik.chain_length);
                if (result.is_err()) {
                    return result.add_message("IKのリンク情報の取得に失敗しました");
                }
            }
        }

        // Logger::info("IK情報の読み込みに成功しました");
        return {};
    }

    PMDModelLoader::Result PMDModelLoader::load_pmd_morphs(
        BinaryReader& binary_reader, PMDData* const pmd_data) {
        std::uint16_t size; // サイズは2Byte

        {
            auto result = binary_reader.read_to(&size);
            if (result.is_err()) {
                return result.add_message("モーフリストサイズの取得に失敗しました");
            }
        }

        pmd_data->morphs.resize(size);
        for (auto& morph : pmd_data->morphs) {
            // 可変部分であるverticesの部分だけは後で読み込むように
            // 先頭25Byteだけ読み込み
            {
                auto result =
                    binary_reader.read(&morph, sizeof(PMDMorph) - sizeof(PMDMorph::vertices));
                if (result.is_err()) {
                    return result.add_message("モーフ情報の取得に失敗しました");
                }
            }

            // 可変部分
            {
                auto result = binary_reader.read_to_vec(morph.vertices, morph.vertex_count);
                if (result.is_err()) {
                    return result.add_message("モーフの頂点情報の取得に失敗しました");
                }
            }
        }

        // Logger::info("モーフ情報の読み込みに成功しました");
        return {};
    }

    PMDModelLoader::Result PMDModelLoader::load_pmd_display_morphs(
        BinaryReader& binary_reader, PMDData* const pmd_data) {
        std::uint8_t size; // サイズは1Byte

        {
            auto result = binary_reader.read_to(&size);
            if (result.is_err()) {
                return result.add_message("モーフの表示リストのサイズ取得に失敗しました");
            }
        }

        {
            auto result = binary_reader.read_to_vec(pmd_data->display_morphs, size);
            if (result.is_err()) {
                return result.add_message("モーフの表示リスト取得に失敗しました");
            }
        }

        // Logger::info("モーフ表示情報の読み込みに成功しました");
        return {};
    }

    PMDModelLoader::Result PMDModelLoader::load_pmd_bone_frame_names(
        BinaryReader& binary_reader, PMDData* const pmd_data) {
        std::uint8_t size; // サイズは1Byte

        {
            auto result = binary_reader.read_to(&size);
            if (result.is_err()) {
                return result.add_message("ボーン枠名リストのサイズ取得に失敗しました");
            }
        }

        {
            auto result = binary_reader.read_to_vec(pmd_data->bone_frame_name, size);
            if (result.is_err()) {
                return result.add_message("ボーン枠名の取得に失敗しました");
            }
        }

        // Logger::info("ボーン枠名情報の読み込みに成功しました");
        return {};
    }

    PMDModelLoader::Result PMDModelLoader::load_pmd_display_bones(
        BinaryReader& binary_reader, PMDData* const pmd_data) {
        std::uint32_t size; // サイズは4Byte

        {
            auto result = binary_reader.read_to(&size);
            if (result.is_err()) {
                return result.add_message("ボーン表示名リストの取得に失敗しました");
            }
        }

        {
            auto result = binary_reader.read_to_vec(pmd_data->display_bones, size);
            if (result.is_err()) {
                return result.add_message("ボーン表示名の取得に失敗しました");
            }
        }

        // Logger::info("ボーン表示名情報の読み込みに成功しました");
        return {};
    }

    PMDModelLoader::Result PMDModelLoader::load_pmd_english_dictionary(
        BinaryReader& binary_reader, PMDData* const pmd_data) {
        auto eng_dict = &pmd_data->english_dictionary;

        {
            auto result = binary_reader.read_to(&eng_dict->is_support);
            if (result.is_err()) {
                return result.add_message("英名対応フラグの取得に失敗しました");
            }
        }

        // 英語非対応の場合はそのまま次へ
        if (eng_dict->is_support == 0) {
            // foundation::Logger::info("このモデルは英名対応していません");
            return {};
        }

        // 順に読み込む
        {
            auto result = binary_reader.read_to(&eng_dict->model_name);
            if (result.is_err()) {
                return result.add_message("モデル名(英名)取得に失敗しました");
            }
        }

        {
            auto result = binary_reader.read_to(&eng_dict->comment);
            if (result.is_err()) {
                return result.add_message("コメント(英名)取得に失敗しました");
            }
        }

        // 可変部分
        {
            auto result = binary_reader.read_to_vec(eng_dict->bone_name, pmd_data->bones.size());
            if (result.is_err()) {
                return result.add_message("ボーン名(英名)取得に失敗しました");
            }
        }

        {
            auto result =
                binary_reader.read_to_vec(eng_dict->skin_name, pmd_data->morphs.size() - 1);
            if (result.is_err()) {
                return result.add_message("スキン名(英名)取得に失敗しました");
            }
        }

        {
            auto result =
                binary_reader.read_to_vec(eng_dict->display_name, pmd_data->bone_frame_name.size());
            if (result.is_err()) {
                return result.add_message("ボーン枠名(英名)取得に失敗しました");
            }
        }

        // Logger::info("英名対応辞書の読み込みに成功しました");
        return {};
    }

    PMDModelLoader::Result PMDModelLoader::load_pmd_toon_textures(
        BinaryReader& binary_reader, PMDData* const pmd_data) {
        {
            auto result = binary_reader.read_to(&pmd_data->toon_textures.file_names);
            if (result.is_err()) {
                return result.add_message("トゥーンテクスチャの取得に失敗しました");
            }
        }

        // Logger::info("トゥーンテクスチャの読み込みに成功しました");
        return {};
    }

    PMDModelLoader::Result PMDModelLoader::load_pmd_rigid_bodies(
        BinaryReader& binary_reader, PMDData* const pmd_data) {
        std::uint32_t size; // サイズは4Byte

        {
            auto result = binary_reader.read_to(&size);
            if (result.is_err()) {
                return result.add_message("剛体情報リストのサイズ読み込みに失敗しました");
            }
        }

        {
            auto result = binary_reader.read_to_vec(pmd_data->rigid_bodies, size);
            if (result.is_err()) {
                return result.add_message("剛体情報の読み込みに失敗しました");
            }
        }

        // Logger::info("剛体情報の読み込みに成功しました");
        return {};
    }

    PMDModelLoader::Result PMDModelLoader::load_pmd_physics_joints(
        BinaryReader& binary_reader, PMDData* const pmd_data) {
        std::uint32_t size; // サイズは4Byte

        {
            auto result = binary_reader.read_to(&size);
            if (result.is_err()) {
                return result.add_message("物理ジョイントリストのサイズ読み込みに失敗しました");
            }
        }

        {
            auto result = binary_reader.read_to_vec(pmd_data->physics_joints, size);
            if (result.is_err()) {
                return result.add_message("物理ジョイント情報の読み込みに失敗しました");
            }
        }

        // Logger::info("物理ジョイント情報の読み込みに成功しました");
        return {};
    }

    foundation::Result<ModelData, AssetError> PMDModelLoader::load(
        const std::filesystem::path& path) noexcept {
        auto reader = BinaryReader::make_reader(path);
        if (!reader.has_value()) {
            return reader.error().propagation(AssetError::IOError);
        }
        auto& binary_reader = reader.value();

        std::unique_ptr<PMDData> pmd_data = std::make_unique<PMDData>();
        auto result = this->load_pmd(binary_reader, pmd_data.get());
        if (result.is_err()) {
            return reader.error().propagation(AssetError::IOError);
        }

        return pmd_data;
    }

    foundation::UTF8 PMDModelLoader::get_supported_extension(void) const noexcept {
        return ".pmd";
    }
} // namespace enishi::assets_system