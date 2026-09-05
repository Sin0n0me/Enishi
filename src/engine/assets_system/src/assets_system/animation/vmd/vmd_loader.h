#pragma once
#include "../../utility/binary_reader.h"
#include "vmd_data.h"
#include "vmd_file_struct.h"
#include <filesystem>
#include <vector>

namespace enishi::assets_system {
    class VMDLoader {
      private:
        using Result = IOResult<void>;

         [[nodiscard]] Result load_vmd(BinaryReader& binary_reader, VMDData* const vmd_data);
         [[nodiscard]] Result load_vmd_header(BinaryReader& binary_reader, VMDData* const vmd_data);
         [[nodiscard]] Result load_vmd_bone_key_frame(BinaryReader& binary_reader, VMDData* const vmd_data);
         [[nodiscard]] Result load_vmd_morph_key_frame(BinaryReader& binary_reader, VMDData* const vmd_data);
         [[nodiscard]] Result load_vmd_camera(BinaryReader& binary_reader, VMDData* const vmd_data);
         [[nodiscard]] Result load_vmd_light(BinaryReader& binary_reader, VMDData* const vmd_data);
         [[nodiscard]] Result load_vmd_shadow(BinaryReader& binary_reader, VMDData* const vmd_data);
         [[nodiscard]] Result load_vmd_ik(BinaryReader& binary_reader, VMDData* const vmd_data);

        explicit VMDLoader(void) noexcept = default;

      public:
        static IOResult<std::unique_ptr<VMDData>> load(const std::filesystem::path& path) noexcept;
    };
} // namespace enishi::assets_system