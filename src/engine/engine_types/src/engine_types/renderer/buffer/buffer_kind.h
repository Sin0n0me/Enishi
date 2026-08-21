#pragma once
#include <cstdint>

namespace enishi::types {
    enum class BufferKind : std::uint32_t {
        Vertex,
        Index,
        Uniform,
    };
} // namespace enishi::types