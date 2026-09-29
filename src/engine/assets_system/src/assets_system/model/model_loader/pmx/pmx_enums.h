#pragma once
#include <cstdint>

namespace enishi::assets_system {
    // Values and underlying types match the PMX binary format.
    enum class PMXDeformType : std::uint8_t {
        BDEF1 = 0,
        BDEF2 = 1,
        BDEF4 = 2,
        SDEF = 3,
        QDEF = 4,
    };

    enum class PMXMorphType : std::uint8_t {
        Group = 0,
        Vertex = 1,
        Bone = 2,
        UV = 3,
        AdditionalUV1 = 4,
        AdditionalUV2 = 5,
        AdditionalUV3 = 6,
        AdditionalUV4 = 7,
        Material = 8,
        Flip = 9,
        Impulse = 10,
    };
} // namespace enishi::assets_system
