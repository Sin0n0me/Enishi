#include <core/system/animation/vertex_morph.h>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <model_controller/model_component_builder.h>

using namespace enishi;

namespace {
    void check(bool condition, const char* message) {
        if (!condition) {
            std::cerr << message << '\n';
            std::exit(EXIT_FAILURE);
        }
    }
} // namespace

int main() {
    types::ModelData source;
    const glm::vec3 base(1, 2, 3);
    source.vertices = {{types::VertexPosition{base}}, {types::VertexPosition{glm::vec3(0)}}};
    types::AddonMorphTargets morphs;
    morphs.targets.push_back(
        {"first", types::MorphWeightMode::Continuous, {types::VertexMorphOffset{0, {2, 0, 0}}}});
    morphs.targets.push_back(
        {"second", types::MorphWeightMode::Continuous, {types::VertexMorphOffset{0, {0, 4, 0}}}});
    source.addons.emplace_back(morphs);
    const auto model = model_controller::make_model_component(source, {});
    check(model.morph_targets.targets[0].name == "first", "model retains target names and indices");
    const std::vector<float> weights{0.5f, 0.25f};
    const auto evaluate = [&](const std::vector<float>& values) {
        return core::evaluate_vertex_morphs(
            model.morph_base_positions, model.morph_targets.targets, values);
    };
    const auto blended = evaluate(weights);
    check(blended.is_ok(), "evaluate morphs");
    check(blended.unwrap()[0] == base + glm::vec3(1, 1, 0),
        "overlapping morphs add weighted offsets");
    check(blended.unwrap()[1] == glm::vec3(0), "unaffected vertex remains unchanged");
    check(
        evaluate(weights).unwrap() == blended.unwrap(), "repeated evaluation does not accumulate");
    check(evaluate({0, 0}).unwrap()[0] == base, "zero weights restore original vertex");
    check(
        evaluate({-1, 0}).unwrap()[0] == base - glm::vec3(2, 0, 0), "signed weights are preserved");
    check(evaluate({1}).is_err(), "weight count mismatch");
    check(evaluate({std::numeric_limits<float>::quiet_NaN(), 0}).is_err(), "non-finite weight");
    morphs.targets[0].offsets = {types::VertexMorphOffset{2, {1, 0, 0}}};
    check(
        core::evaluate_vertex_morphs(model.morph_base_positions, morphs.targets, weights).is_err(),
        "invalid vertex must return an error");
    std::cout << "Vertex morph tests passed\n";
}
