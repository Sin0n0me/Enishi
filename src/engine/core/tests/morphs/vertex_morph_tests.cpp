#include <core/system/animation/vertex_morph.h>
#include <core/system/render/vertex_morph_upload.h>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <limits>
#include <model_controller/model_component_builder.h>
#include <renderer/common/converter/skinned_vertices.h>
#include <renderer/common/vertex_buffer_updater.h>

using namespace enishi;

namespace {
    void check(bool condition, const char* message) {
        if (!condition) {
            std::cerr << message << '\n';
            std::exit(EXIT_FAILURE);
        }
    }
} // namespace

void morph_weight_tests();
void uv_morph_tests();

int main() {
    morph_weight_tests();
    uv_morph_tests();
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
    std::vector<renderer::SkinnedVertex> vertices(source.vertices.size());
    vertices[0].normal = {0, 1, 0};
    vertices[0].weights = {0.25f, 0.75f, 0, 0};
    bool uploaded = false;
    renderer::VertexBufferUpdater updater(
        types::OwnedRenderData(vertices), [&](const types::RenderData& bytes) {
            renderer::SkinnedVertex actual;
            std::memcpy(&actual, bytes.raw_data(), sizeof(actual));
            check(actual.position == blended.unwrap()[0], "morph position reaches upload callback");
            check(actual.normal == vertices[0].normal && actual.weights == vertices[0].weights,
                "upload preserves normal and skin weights");
            uploaded = true;
        });
    check(core::write_vertex_positions(
              updater, offsetof(renderer::SkinnedVertex, position), blended.unwrap())
                  .is_ok() &&
              uploaded,
        "vertex position upload");
    uploaded = false;
    check(core::write_vertex_positions(updater, sizeof(renderer::SkinnedVertex), blended.unwrap())
                  .is_err() &&
              !uploaded,
        "bad position offset must not upload");
    check(core::write_vertex_positions(updater, 0, {}).is_err() && !uploaded,
        "wrong vertex count must not upload");
    morphs.targets[0].offsets = {types::VertexMorphOffset{2, {1, 0, 0}}};
    check(
        core::evaluate_vertex_morphs(model.morph_base_positions, morphs.targets, weights).is_err(),
        "invalid vertex must return an error");
    std::cout << "Vertex morph tests passed\n";
}
