#include "uv_morph.h"
#include <cmath>

namespace enishi::core {
    foundation::Result<UVChannels, MorphError> evaluate_uv_morphs(const UVChannels& base,
        std::span<const types::MorphTarget> targets,
        std::span<const float> weights) {
        const auto effective = evaluate_morph_weights(targets, weights);
        if (effective.is_err()) {
            return effective.propagation(MorphError::InvalidWeight);
        }
        auto channels = base;
        for (std::size_t index = 0; index < targets.size(); ++index) {
            const auto weight = effective.unwrap()[index];
            if (weight == 0.0f) {
                continue;
            }
            for (const auto& offset : targets[index].offsets) {
                const auto* uv = std::get_if<types::UVMorphOffset>(&offset);
                if (uv == nullptr) {
                    continue;
                }
                if (!(uv->channel < channels.size()) ||
                    !(uv->vertex < channels[uv->channel].size())) {
                    return foundation::Error(
                        MorphError::InvalidReference, "UV morph channel or vertex out of range");
                }
                auto& value = channels[uv->channel][uv->vertex];
                value += uv->offset * weight;
                for (glm::length_t component = 0; component < value.length(); ++component) {
                    if (!std::isfinite(value[component])) {
                        return foundation::Error(
                            MorphError::InvalidWeight, "Non-finite UV morph value");
                    }
                }
            }
        }
        return channels;
    }
} // namespace enishi::core
