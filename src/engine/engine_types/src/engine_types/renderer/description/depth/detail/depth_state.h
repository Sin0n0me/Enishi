#pragma once
#include "compare_operator.h"
#include <cstdint>

namespace enishi::types {
    struct DepthState {
        bool enabled;
        bool write_enabled;
        CompareOperator compare_operator;
    };
} // namespace enishi::types