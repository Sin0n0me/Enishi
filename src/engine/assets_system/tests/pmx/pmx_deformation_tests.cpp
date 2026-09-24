#include <assets_system/model/model_loader/pmx/pmx_to_model_data.h>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <renderer/common/converter/skinned_vertices.h>

using namespace enishi;
using namespace enishi::assets_system;

namespace {
    constexpr std::uint8_t BDEF1 = 0;
    constexpr std::uint8_t BDEF2 = 1;
    constexpr std::uint8_t BDEF4 = 2;
    constexpr std::uint8_t SDEF = 3;
    constexpr std::uint8_t QDEF = 4;
    constexpr std::size_t SKINNING_ATTRIBUTE = 1;
    constexpr float TOLERANCE = 0.000001f;

    void check(bool condition, const char* message) {
        if (!condition) {
            std::cerr << message << '\n';
            std::exit(EXIT_FAILURE);
        }
    }

    PMXData deformation_fixture() {
        PMXData data;
        data.version = 2.1f;
        data.bones.resize(4);
        for (const auto method : {BDEF1, BDEF2, BDEF4, SDEF, QDEF}) {
            auto& vertex = data.vertices.emplace_back();
            vertex.deform_type = method;
            vertex.position = {1, 2, 3};
            vertex.normal = {0, 1, 0};
            vertex.bones = {0, 1, -1, -1};
            vertex.weights = {0.25f, 0.75f, 0, 0};
            if (method == BDEF1) {
                vertex.weights = {1, 0, 0, 0};
                vertex.bones[1] = -1;
            }
            if (method == BDEF4 || method == QDEF) {
                vertex.bones = {0, 1, 2, 3};
                vertex.weights = {1, 2, 3, 4};
            }
            if (method == SDEF) {
                vertex.sdef_center = {1, 2, 3};
                vertex.sdef_radius0 = {8, 0, 0};
                vertex.sdef_radius1 = {0, 0, 0};
            }
        }
        return data;
    }

    void mixed_deformation_test() {
        const auto data = deformation_fixture();
        const auto result = PMXToModelData::to_model_data("mixed.pmx", data, nullptr);
        check(result.is_ok(), "mixed BDEF1/BDEF2/BDEF4/SDEF/QDEF conversion");
        const auto& model = *result.unwrap();
        check(model.skinning_methods ==
                  std::vector<types::SkinningMethod>{types::SkinningMethod::LinearBlend,
                      types::SkinningMethod::LinearBlend,
                      types::SkinningMethod::LinearBlend,
                      types::SkinningMethod::SphericalBlend,
                      types::SkinningMethod::DualQuaternion},
            "deformation methods must remain distinct");
        for (const auto& vertex : model.vertices) {
            check(std::holds_alternative<types::Skinning4>(vertex[SKINNING_ATTRIBUTE]),
                "mixed model has a consistent four-influence layout");
        }
        const auto& blend = model.spherical_blends[SDEF];
        check(blend.center == glm::vec3(1, 2, 3) && blend.anchor0 == glm::vec3(4, 2, 3) &&
                  blend.anchor1 == glm::vec3(0, 2, 3),
            "SDEF becomes corrected bind-space anchors");
        const auto packed = renderer::make_skinned_vertices(model);
        check(packed.is_ok(), "PMX model must reach the GPU vertex layout");
        const auto& gpu = packed.unwrap();
        check(gpu[BDEF1].bones == glm::uvec4(0),
            "unused references must not reach GPU array lookups");
        check(
            gpu[BDEF2].weights == glm::vec4(0.25f, 0.75f, 0, 0), "legacy weights in a mixed model");
        for (const auto method : {BDEF4, QDEF}) {
            check(
                glm::length(gpu[method].weights - glm::vec4(0.1f, 0.2f, 0.3f, 0.4f)) < TOLERANCE &&
                    gpu[method].bones == glm::uvec4(0, 1, 2, 3),
                "four normalized influences reach GPU");
        }
        check(gpu[SDEF].anchor0 == blend.anchor0 && gpu[SDEF].anchor1 == blend.anchor1,
            "spherical anchors reach GPU unchanged");
    }

    void malformed_deformation_test() {
        for (const auto method : {BDEF4, QDEF}) {
            auto data = deformation_fixture();
            data.vertices[method].weights = {};
            auto result = PMXToModelData::to_model_data("invalid.pmx", data, nullptr);
            check(
                result.is_err() && result.unwrap_err().get_error() == AssetError::InvalidAssetData,
                "zero total weight rejected");
            data = deformation_fixture();
            data.vertices[method].bones[3] = -1;
            result = PMXToModelData::to_model_data("invalid.pmx", data, nullptr);
            check(result.is_err() &&
                      result.unwrap_err().get_error() == AssetError::UnsupportedFeature,
                "weighted missing fourth influence rejected");
            data.vertices[method].weights[3] = 0;
            check(PMXToModelData::to_model_data("valid.pmx", data, nullptr).is_ok(),
                "unused missing fourth influence accepted");
        }
        auto data = deformation_fixture();
        data.vertices[SDEF].sdef_center[0] = std::numeric_limits<float>::quiet_NaN();
        check(PMXToModelData::to_model_data("invalid.pmx", data, nullptr).is_err(),
            "nonfinite spherical parameters rejected");
    }
} // namespace

void pmx_deformation_tests() {
    mixed_deformation_test();
    malformed_deformation_test();
}
