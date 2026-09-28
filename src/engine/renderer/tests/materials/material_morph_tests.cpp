#include <core/system/animation/material_morph.h>
#include <core/system/render/material_morph_upload.h>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <limits>
#include <renderer/common/vertex_buffer_updater.h>

using namespace enishi;

namespace {
    constexpr float TOLERANCE = 0.0001f;
    void check(bool value, const char* message) {
        if (!value) {
            std::cerr << message << '\n';
            std::exit(EXIT_FAILURE);
        }
    }
} // namespace

void material_morph_tests() {
    std::vector<types::Material> materials(2);
    for (auto& material : materials) {
        material.name = types::UniformMaterial::UNIFORM_NAME;
        material.variants = {types::Diffuse{{0.2f, 0.4f, 0.6f, 0.8f}}};
        material.outline_width = 2;
    }
    std::vector<types::MorphTarget> targets(3);
    targets[0].offsets = {types::MaterialMorphOffset{UINT32_MAX,
        types::MorphOperation::Multiply,
        {{"diffuse", glm::vec4(2)},
            {"outline_width", glm::vec4(2)},
            {"base_color_texture_tint", glm::vec4(0.5f)}}}};
    targets[1].offsets = {types::MaterialMorphOffset{0,
        types::MorphOperation::Add,
        {{"diffuse", {0.2f, 0.4f, 0.6f, -0.2f}}, {"base_color_texture_tint", glm::vec4(0.1f)}}}};
    targets[2].offsets = {types::MorphWeightOffset{0, 0.5f}};
    auto weights = std::vector<float>{0, 0.5f, 1};
    auto result = core::evaluate_material_morphs(materials, targets, weights);
    check(result.is_ok(), "evaluate grouped material morphs");
    const auto& first = result.unwrap()[0];
    check(glm::length(first.diffuse - glm::vec4(0.4f, 0.8f, 1.2f, 1.1f)) < TOLERANCE,
        "material result is base times accumulated multiplication plus addition");
    check(glm::length(result.unwrap()[1].diffuse - glm::vec4(0.3f, 0.6f, 0.9f, 1.2f)) < TOLERANCE,
        "all-material offsets apply while individual offsets stay isolated");
    check(first.outline_parameters.x == 3 && first.base_color_texture_factor == glm::vec4(0.75f) &&
              first.base_color_texture_add == glm::vec4(0.05f),
        "outline width and texture color operations remain distinct");
    constexpr std::size_t BUFFER_SIZE = 256;
    bool uploaded = false;
    renderer::VertexBufferUpdater updater(
        types::OwnedRenderData(std::vector<std::byte>(BUFFER_SIZE, std::byte{0x5a}), BUFFER_SIZE),
        [&](const types::RenderData& data) {
            types::UniformMaterial actual;
            std::memcpy(&actual, data.raw_data(), sizeof(actual));
            check(actual.diffuse == first.diffuse &&
                      actual.outline_parameters == first.outline_parameters,
                "material color and outline reach GPU upload callback");
            check(data.bytes.back() == std::byte{0x5a}, "material upload preserves buffer padding");
            uploaded = true;
        });
    check(core::write_material_uniform(updater, first).is_ok() && uploaded,
        "upload evaluated material");
    renderer::VertexBufferUpdater small(
        types::OwnedRenderData(std::vector<std::byte>(sizeof(first) - 1), sizeof(first) - 1),
        [&](const types::RenderData&) { check(false, "invalid material buffer must not upload"); });
    check(core::write_material_uniform(small, first).is_err(), "reject short material buffer");
    weights = {0.5f, 0.5f, 0};
    const auto before =
        core::evaluate_material_morphs(materials, targets, weights).unwrap()[0].diffuse;
    std::swap(targets[0], targets[1]);
    check(core::evaluate_material_morphs(materials, targets, weights).unwrap()[0].diffuse == before,
        "additive and multiplicative morph order does not change the result");
    weights = {0, 0, 0};
    result = core::evaluate_material_morphs(materials, targets, weights);
    check(result.is_ok() && result.unwrap()[0].diffuse == glm::vec4(0.2f, 0.4f, 0.6f, 0.8f),
        "zero weights restore original material");
    targets[0].offsets = {
        types::MaterialMorphOffset{2, types::MorphOperation::Add, {{"diffuse", glm::vec4(1)}}}};
    weights[0] = 1;
    check(core::evaluate_material_morphs(materials, targets, weights).is_err(),
        "out of range material rejected");
    targets[0].offsets = {
        types::MaterialMorphOffset{0, types::MorphOperation::Add, {{"unknown", glm::vec4(1)}}}};
    check(core::evaluate_material_morphs(materials, targets, weights).is_err(),
        "unknown material property rejected");
    targets[0].offsets = {types::MaterialMorphOffset{0,
        types::MorphOperation::Add,
        {{"diffuse", glm::vec4(std::numeric_limits<float>::quiet_NaN())}}}};
    check(core::evaluate_material_morphs(materials, targets, weights).is_err(),
        "non-finite material value rejected");
}
