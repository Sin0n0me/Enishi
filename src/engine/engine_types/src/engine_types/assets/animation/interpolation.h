#pragma once

namespace enishi::types {
    enum class InterolationTag {};

    union InterolationUnion {};

    struct Interolation {
        InterolationTag tag;
    };
} // namespace enishi::types