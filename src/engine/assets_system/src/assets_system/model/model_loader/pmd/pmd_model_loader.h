#pragma once
#include "../../../utility/binary_reader.h"
#include "../interface_model_loader.h"
#include "pmd_data.h"
#include "pmd_file_struct.h"
#include <filesystem>
#include <memory>

namespace enishi::assets_system {
    class PMDModelLoader : public IModelLoader {
      private:
        using Result = IOResult<void>;

        [[nodiscard]] Result load_pmd(BinaryReader& binary_reader, PMDData* const pmd_data);
        [[nodiscard]] Result load_pmd_header(BinaryReader& binary_reader, PMDData* const pmd_data);
        [[nodiscard]] Result load_pmd_vertices(
            BinaryReader& binary_reader, PMDData* const pmd_data);
        [[nodiscard]] Result load_pmd_indices(BinaryReader& binary_reader, PMDData* const pmd_data);
        [[nodiscard]] Result load_pmd_materials(
            BinaryReader& binary_reader, PMDData* const pmd_data);
        [[nodiscard]] Result load_pmd_bones(BinaryReader& binary_reader, PMDData* const pmd_data);
        [[nodiscard]] Result load_pmd_iks(BinaryReader& binary_reader, PMDData* const pmd_data);
        [[nodiscard]] Result load_pmd_morphs(BinaryReader& binary_reader, PMDData* const pmd_data);
        [[nodiscard]] Result load_pmd_display_morphs(
            BinaryReader& binary_reader, PMDData* const pmd_data);
        [[nodiscard]] Result load_pmd_bone_frame_names(
            BinaryReader& binary_reader, PMDData* const pmd_data);
        [[nodiscard]] Result load_pmd_display_bones(
            BinaryReader& binary_reader, PMDData* const pmd_data);
        [[nodiscard]] Result load_pmd_english_dictionary(
            BinaryReader& binary_reader, PMDData* const pmd_data);
        [[nodiscard]] Result load_pmd_toon_textures(
            BinaryReader& binary_reader, PMDData* const pmd_data);
        [[nodiscard]] Result load_pmd_rigid_bodies(
            BinaryReader& binary_reader, PMDData* const pmd_data);
        [[nodiscard]] Result load_pmd_physics_joints(
            BinaryReader& binary_reader, PMDData* const pmd_data);

      public:
        explicit PMDModelLoader(void) = default;
        foundation::Result<ModelVariant, AssetError> load(
            const std::filesystem::path& path) noexcept override;
        foundation::UTF8 get_supported_extension(void) const noexcept override;
    };
} // namespace enishi::assets_system
