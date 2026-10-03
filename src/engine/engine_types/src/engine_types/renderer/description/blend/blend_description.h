#pragma once
#include "detail/render_target_blend_state.h"

namespace enishi::types {
    constexpr std::uint32_t MAX_REDNER_TARGETS = 8;

    struct BlendStateDescription {
        bool alpha_to_coverage;
        bool independent_blend;
        RenderTargetBlendState render_targets[MAX_REDNER_TARGETS];
    };
} // namespace enishi::types