#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace enishi::types {
    enum class ShaderBinaryType {
        SPIR_V,
        DXIL,
        DXBC,

        SourceFileHLSL,
        SourceFileGLSL,
        SourceFileMSL,
    };

    struct ShaderData {
        ShaderBinaryType binary_type;
        std::vector<std::uint8_t> code;
    };
} // namespace enishi::types