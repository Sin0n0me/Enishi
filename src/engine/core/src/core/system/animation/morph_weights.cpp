#include "morph_weights.h"
#include <cmath>

namespace enishi::core {
    namespace {
        bool is_selection(const types::MorphTarget& target) {
            return target.weight_mode == types::MorphWeightMode::DiscreteSelection;
        }

        foundation::Result<void, MorphError> validate_weights(
            std::span<const types::MorphTarget> targets, std::span<const float> weights) {
            if (targets.size() != weights.size()) {
                return foundation::Error(
                    MorphError::InvalidWeight, "Morph weight count differs from target count");
            }
            std::vector<std::size_t> incoming(targets.size());
            for (std::size_t index = 0; index < targets.size(); ++index) {
                if (!std::isfinite(weights[index])) {
                    return foundation::Error(MorphError::InvalidWeight, "Non-finite morph weight");
                }
                for (const auto& offset : targets[index].offsets) {
                    const auto* reference = std::get_if<types::MorphWeightOffset>(&offset);
                    if (reference == nullptr) {
                        if (is_selection(targets[index])) {
                            return foundation::Error(MorphError::UnsupportedOffset,
                                "Selection requires weight references");
                        }
                        continue;
                    }
                    if (!(reference->morph < targets.size()) || !std::isfinite(reference->weight)) {
                        return foundation::Error(
                            MorphError::InvalidReference, "Invalid morph weight reference");
                    }
                    if (!is_selection(targets[index])) {
                        ++incoming[reference->morph];
                    }
                }
            }
            std::vector<std::size_t> ready;
            for (std::size_t index = 0; index < incoming.size(); ++index) {
                if (incoming[index] == 0) {
                    ready.push_back(index);
                }
            }
            for (std::size_t cursor = 0; cursor < ready.size(); ++cursor) {
                const auto& target = targets[ready[cursor]];
                if (is_selection(target)) {
                    continue;
                }
                for (const auto& offset : target.offsets) {
                    const auto* reference = std::get_if<types::MorphWeightOffset>(&offset);
                    if (reference != nullptr && --incoming[reference->morph] == 0) {
                        ready.push_back(reference->morph);
                    }
                }
            }
            if (ready.size() != targets.size()) {
                return foundation::Error(
                    MorphError::CyclicReference, "Cyclic group morph references");
            }
            return {};
        }

        void select_weight(
            const types::MorphTarget& target, float weight, std::vector<float>& values) {
            if (!(weight > 0.0f) || target.offsets.empty()) {
                return;
            }
            const auto selected = (static_cast<double>(target.offsets.size()) + 1.0) * weight;
            if (selected < 1.0) {
                return;
            }
            const auto index = selected > static_cast<double>(target.offsets.size())
                                   ? target.offsets.size() - 1
                                   : static_cast<std::size_t>(selected) - 1;
            const auto& reference = std::get<types::MorphWeightOffset>(target.offsets[index]);
            values[reference.morph] = reference.weight;
        }

        struct Contribution {
            std::size_t morph;
            float weight;
        };

        foundation::Result<void, MorphError> expand_weights(
            std::span<const types::MorphTarget> targets,
            Contribution root,
            std::vector<float>& output,
            bool selections) {
            // The graph has already been checked. An explicit stack avoids recursion
            // depth limits while preserving source order for selection overwrites.
            std::vector<Contribution> pending{root};
            while (!pending.empty()) {
                const auto current = pending.back();
                pending.pop_back();
                if (!std::isfinite(current.weight)) {
                    return foundation::Error(
                        MorphError::InvalidWeight, "Morph weight product overflow");
                }
                if (current.weight == 0.0f) {
                    continue;
                }
                const auto& target = targets[current.morph];
                if (is_selection(target)) {
                    if (selections) {
                        select_weight(target, current.weight, output);
                    }
                    continue;
                }
                if (!selections) {
                    output[current.morph] += current.weight;
                    if (!std::isfinite(output[current.morph])) {
                        return foundation::Error(
                            MorphError::InvalidWeight, "Morph weight sum overflow");
                    }
                }
                for (auto offset = target.offsets.rbegin(); offset != target.offsets.rend();
                     ++offset) {
                    if (const auto* reference = std::get_if<types::MorphWeightOffset>(&*offset);
                        reference != nullptr) {
                        pending.push_back({reference->morph, current.weight * reference->weight});
                    }
                }
            }
            return {};
        }
    } // namespace

    foundation::Result<std::vector<float>, MorphError> evaluate_morph_weights(
        std::span<const types::MorphTarget> targets, std::span<const float> weights) {
        auto valid = validate_weights(targets, weights);
        if (valid.is_err()) {
            return valid.propagation(static_cast<MorphError>(valid.unwrap_err().get_error()));
        }
        std::vector<float> selected(weights.begin(), weights.end());
        for (std::size_t index = 0; index < targets.size(); ++index) {
            auto result = expand_weights(targets, {index, selected[index]}, selected, true);
            if (result.is_err()) {
                return result.propagation(MorphError::InvalidWeight);
            }
        }
        std::vector<float> effective(targets.size());
        for (std::size_t index = 0; index < targets.size(); ++index) {
            auto result = expand_weights(targets, {index, selected[index]}, effective, false);
            if (result.is_err()) {
                return result.propagation(MorphError::InvalidWeight);
            }
        }
        return effective;
    }
} // namespace enishi::core
