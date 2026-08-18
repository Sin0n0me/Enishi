#pragma once
#include <cstdint>
#include <variant>

namespace enishi::types {
    struct Draw {
        std::uint32_t vertex_count;
        std::uint32_t instance_count;
        std::uint32_t first_vertex;
        std::uint32_t first_instance;
    };

    struct DrawIndexed {
        std::uint32_t index_count;
        std::uint32_t instance_count;
        std::int32_t first_index;
        std::uint32_t vertex_offset;
        std::uint32_t first_instance;
    };

    using DrawArgs = std::variant<Draw, DrawIndexed>;
} // namespace enishi::types
