#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace enishi::types {
    enum class ShaderBinaryType : std::uint8_t {
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

        std::size_t hash(void) const;

        bool operator==(const ShaderData& other) const;
    };
} // namespace enishi::types
