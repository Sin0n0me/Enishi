#include "vrm_tests.h"
#include "../../../core/tests/test_runner.h"
#include "vrm_fixture.h"
#include <assets_system/model/model_loader/vrm/vrm_model_loader.h>
#include <cmath>

namespace enishi::tests {
    namespace {
        constexpr float TOLERANCE = 0.0001f;
        constexpr std::size_t TRIANGLE_VERTICES = 3;
        constexpr std::size_t UPPER_ARM = 1;
        constexpr std::size_t LOWER_ARM = 2;
        constexpr std::size_t POSITION_ATTRIBUTE = 0;
        constexpr glm::length_t TRANSLATION_COLUMN = 3;
        constexpr float EXPECTED_UNITS_PER_METER = 10.0f;
        constexpr float MORPH_DISPLACEMENT_METERS = 0.1f;
        constexpr float EXPRESSION_WEIGHT = 0.5f;
        constexpr std::size_t MORPH_TARGET_COUNT = 2;

        template <typename Addon> const Addon& addon(const types::ModelData& model) {
            for (const auto& value : model.addons) {
                if (const auto* found = std::get_if<Addon>(&value); found != nullptr) {
                    return *found;
                }
            }
            check(false, "expected model addon");
            std::exit(EXIT_FAILURE);
        }
    } // namespace

    void vrm_loading_tests(bool legacy) {
        VRMFixture fixture(legacy);
        assets_system::VRMModelLoader loader;
        auto result = loader.load(fixture.write());
        if (result.is_err()) {
            check(false, result.unwrap_err().get_message().c_str());
        }
        check(result.is_ok(), "load synthetic VRM");
        const auto& model = *std::get<types::AssetModelData>(result.unwrap());
        check(model.vertices.size() == TRIANGLE_VERTICES, "triangle vertex count");
        check(!model.vertices[1].empty(), "vertex has a position attribute");
        const auto& vertex = std::get<types::Vertex>(model.vertices[1][POSITION_ATTRIBUTE]);
        const float expected_x = legacy ? EXPECTED_UNITS_PER_METER : -EXPECTED_UNITS_PER_METER;
        check(std::abs(vertex.position.x - expected_x) < TOLERANCE,
            "VRM version-specific facing and meter conversion");
        check(std::abs(glm::length(vertex.normal) - 1.0f) < TOLERANCE,
            "generated triangle normals are normalized");
        const auto& bones = addon<types::AddonBones>(model);
        check(LOWER_ARM < bones.size(), "all fixture nodes imported as bones");
        check(bones[UPPER_ARM].name == "leftUpperArm", "humanoid role replaces node name");
        check(bones[LOWER_ARM].bone_node.parent == UPPER_ARM, "bone hierarchy preserved");
        check(std::abs(bones[UPPER_ARM].bind_bone.global[TRANSLATION_COLUMN].y -
                       EXPECTED_UNITS_PER_METER) < TOLERANCE,
            "bone translations use model units");
        const auto& targets = addon<types::AddonMorphTargets>(model).targets;
        check(targets.size() == MORPH_TARGET_COUNT, "vertex morph and expression group imported");
        check(!targets.front().offsets.empty(), "vertex morph has offsets");
        const auto& offset = std::get<types::VertexMorphOffset>(targets.front().offsets.front());
        // Model displacement = meter displacement * signed model units per meter.
        check(std::abs(offset.translation.x - expected_x * MORPH_DISPLACEMENT_METERS) < TOLERANCE,
            "morph displacement uses the same coordinate conversion");
        check(targets.back().name == "happy" && targets.back().expression,
            "VRM 0 preset aliases and VRM 1 expression names agree");
        check(targets.back().weight_mode == types::MorphWeightMode::Binary,
            "binary expression flag preserved");
        check(!targets.back().offsets.empty(), "expression has morph bindings");
        const auto& bind = std::get<types::MorphWeightOffset>(targets.back().offsets.front());
        check(bind.morph == 0, "expression references the vertex morph");
        check(std::abs(bind.weight - EXPRESSION_WEIGHT) < TOLERANCE,
            "percent and unit expression weights agree");
        const auto& springs = addon<types::AddonSpringBones>(model);
        check(!springs.chains.empty() && !springs.chains.front().joints.empty(),
            "spring chains imported for both VRM versions");
        check(springs.chains.front().joints.front().bone == UPPER_ARM,
            "spring references the expected bone");
    }

    void vrm_rejection_tests() {
        assets_system::VRMModelLoader loader;
        VRMFixture fixture(false);
        fixture.required_extensions = R"(,"extensionsRequired":["UNSUPPORTED_test_extension"] )";
        check(loader.load(fixture.write()).is_err(), "reject unsupported required extensions");
        fixture.required_extensions.clear();
        fixture.buffer_uri = R"(,"uri":"missing-external-buffer.bin")";
        check(
            loader.load(fixture.write()).is_err(), "reject external buffers without reading them");
        fixture.buffer_uri.clear();
        fixture.extension = R"("VRMC_vrm":{"specVersion":"9.0"})";
        check(loader.load(fixture.write()).is_err(), "reject unsupported VRM versions");
        fixture.extension =
            R"("VRMC_vrm":{"specVersion":"1.0","humanoid":{"humanBones":{"leftUpperArm":{"node":999}}}})";
        check(loader.load(fixture.write()).is_err(), "reject out-of-range humanoid nodes");
    }
} // namespace enishi::tests

int main() {
    enishi::tests::run_test("VRM 0 conversion", [] { enishi::tests::vrm_loading_tests(true); });
    enishi::tests::run_test("VRM 1 conversion", [] { enishi::tests::vrm_loading_tests(false); });
    enishi::tests::run_test("VRM invalid input", enishi::tests::vrm_rejection_tests);
}
