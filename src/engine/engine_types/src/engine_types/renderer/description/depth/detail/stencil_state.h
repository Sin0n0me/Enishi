#pragma once
#include "compare_operator.h"
#include "stencil_operator.h"
#include "stencil_state.h"

namespace enishi::types {
    struct StencilFaceState {
        StencilOpeartor stencil_fail_operator;
        StencilOpeartor depth_fail_operator;
        StencilOpeartor pass_operator;
        CompareOperator compare_operator;
        std::uint32_t compare_mask;
        std::uint32_t write_mask;
        std::uint32_t reference;
    };

    struct StencilState {
        bool enabled;

        StencilFaceState front;
        StencilFaceState back;
    };
} // namespace enishi::types