#include <cmath>
#include <core/system/animation/morph_weights.h>
#include <cstdlib>
#include <iostream>
#include <limits>

using namespace enishi;

namespace {
    void check(bool value, const char* message) {
        if (!value) {
            std::cerr << message << '\n';
            std::exit(EXIT_FAILURE);
        }
    }
} // namespace

void morph_weight_tests() {
    constexpr float tolerance = 0.00001f;
    std::vector<types::MorphTarget> targets(4);
    targets[0].offsets = {types::VertexMorphOffset{0, {2, 0, 0}}};
    targets[1].offsets = {types::MorphWeightOffset{0, 0.5f}};
    targets[2].offsets = {types::MorphWeightOffset{1, 0.25f}};
    auto weights = std::vector<float>{0.1f, 0.2f, 0.8f, 0};
    auto result = core::evaluate_morph_weights(targets, weights);
    check(result.is_ok() && std::abs(result.unwrap()[0] - 0.3f) < tolerance,
        "nested and direct group weights add");
    const std::vector<glm::vec3> positions{{0, 0, 0}};
    const auto vertices = core::evaluate_vertex_morphs(positions, targets, weights);
    check(vertices.is_ok() && std::abs(vertices.unwrap()[0].x - 0.6f) < tolerance,
        "group morphs reach vertex deformation");

    targets[3].weight_mode = types::MorphWeightMode::DiscreteSelection;
    targets[3].offsets = {types::MorphWeightOffset{0, 0.75f}, types::MorphWeightOffset{0, -0.5f}};
    weights = {1, 0, 0, 0.5f};
    result = core::evaluate_morph_weights(targets, weights);
    check(result.is_ok() && result.unwrap()[0] == 0.75f,
        "flip replaces rather than multiplies weight");
    weights[3] = 1;
    check(core::evaluate_morph_weights(targets, weights).unwrap()[0] == -0.5f,
        "flip endpoint selects final entry");
    weights[3] = 100;
    check(core::evaluate_morph_weights(targets, weights).unwrap()[0] == -0.5f,
        "flip above one clamps selection");
    weights[3] = -1;
    check(core::evaluate_morph_weights(targets, weights).unwrap()[0] == 1,
        "negative flip weight selects nothing");
    targets[1].offsets = {types::MorphWeightOffset{3, 0.5f}};
    weights = {1, 1, 0, 0};
    check(core::evaluate_morph_weights(targets, weights).unwrap()[0] == 0.75f,
        "group can activate a flip during selection pass");
    targets[3].offsets = {types::MorphWeightOffset{3, 1}};
    weights = {0, 0, 0, 1};
    check(core::evaluate_morph_weights(targets, weights).is_ok(),
        "self-selecting flip does not recurse");
    targets[1].offsets = {types::MorphWeightOffset{2, 1}};
    check(core::evaluate_morph_weights(targets, weights).is_err(),
        "group cycles are rejected even at zero weight");
    targets[1].offsets = {types::MorphWeightOffset{99, 1}};
    check(
        core::evaluate_morph_weights(targets, weights).is_err(), "invalid references are rejected");
    targets[1].offsets = {types::MorphWeightOffset{0, std::numeric_limits<float>::max()}};
    weights = {0, std::numeric_limits<float>::max(), 0, 0};
    check(core::evaluate_morph_weights(targets, weights).is_err(),
        "weight multiplication overflow is rejected");
}
