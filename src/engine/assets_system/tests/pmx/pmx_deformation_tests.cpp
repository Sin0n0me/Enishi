#include <assets_system/model/model_loader/pmx/pmx_to_model_data.h>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <renderer/common/converter/skinned_vertices.h>

using namespace enishi;
using namespace enishi::assets_system;

namespace {
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
        for (const auto method : {PMXDeformType::BDEF1,
                 PMXDeformType::BDEF2,
                 PMXDeformType::BDEF4,
                 PMXDeformType::SDEF,
                 PMXDeformType::QDEF}) {
            auto& vertex = data.vertices.emplace_back();
            vertex.deform_type = method;
            vertex.position = {1, 2, 3};
            vertex.normal = {0, 1, 0};
            vertex.bones = {0, 1, -1, -1};
            vertex.weights = {0.25f, 0.75f, 0, 0};
            if (method == PMXDeformType::BDEF1) {
                vertex.weights = {1, 0, 0, 0};
                vertex.bones[1] = -1;
            }
            if (method == PMXDeformType::BDEF4 || method == PMXDeformType::QDEF) {
                vertex.bones = {0, 1, 2, 3};
                vertex.weights = {1, 2, 3, 4};
            }
            if (method == PMXDeformType::SDEF) {
                vertex.sdef_center = {1, 2, 3};
                vertex.sdef_radius0 = {8, 0, 0};
                vertex.sdef_radius1 = {0, 0, 0};
            }
        }
        return data;
    }

    void mixed_deformation_test() {
        auto data = deformation_fixture();
        data.additional_uv_count = 4;
        for (auto& vertex : data.vertices) {
            vertex.additional_uvs.resize(data.additional_uv_count, {1, 2, 3, 4});
        }
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
        const auto& blend = model.spherical_blends[static_cast<std::size_t>(PMXDeformType::SDEF)];
        check(blend.center == glm::vec3(1, 2, 3) && blend.anchor0 == glm::vec3(4, 2, 3) &&
                  blend.anchor1 == glm::vec3(0, 2, 3),
            "SDEF becomes corrected bind-space anchors");
        const auto packed = renderer::make_skinned_vertices(model);
        check(packed.is_ok(), "PMX model must reach the GPU vertex layout");
        const auto& gpu = packed.unwrap();
        check(gpu.front().additional_uvs.back() == glm::vec4(1, 2, 3, 4),
            "additional PMX UV channels survive conversion and packing");
        check(gpu[static_cast<std::size_t>(PMXDeformType::BDEF1)].bones == glm::uvec4(0),
            "unused references must not reach GPU array lookups");
        check(gpu[static_cast<std::size_t>(PMXDeformType::BDEF2)].weights ==
                  glm::vec4(0.25f, 0.75f, 0, 0),
            "legacy weights in a mixed model");
        for (const auto method : {PMXDeformType::BDEF4, PMXDeformType::QDEF}) {
            check(glm::length(gpu[static_cast<std::size_t>(method)].weights -
                              glm::vec4(0.1f, 0.2f, 0.3f, 0.4f)) < TOLERANCE &&
                      gpu[static_cast<std::size_t>(method)].bones == glm::uvec4(0, 1, 2, 3),
                "four normalized influences reach GPU");
        }
        check(gpu[static_cast<std::size_t>(PMXDeformType::SDEF)].anchor0 == blend.anchor0 &&
                  gpu[static_cast<std::size_t>(PMXDeformType::SDEF)].anchor1 == blend.anchor1,
            "spherical anchors reach GPU unchanged");
    }

    void malformed_deformation_test() {
        for (const auto method : {PMXDeformType::BDEF4, PMXDeformType::QDEF}) {
            auto data = deformation_fixture();
            data.vertices[static_cast<std::size_t>(method)].weights = {};
            auto result = PMXToModelData::to_model_data("invalid.pmx", data, nullptr);
            check(
                result.is_err() && result.unwrap_err().get_error() == AssetError::InvalidAssetData,
                "zero total weight rejected");
            data = deformation_fixture();
            data.vertices[static_cast<std::size_t>(method)].bones[3] = -1;
            result = PMXToModelData::to_model_data("invalid.pmx", data, nullptr);
            check(result.is_err() &&
                      result.unwrap_err().get_error() == AssetError::UnsupportedFeature,
                "weighted missing fourth influence rejected");
            data.vertices[static_cast<std::size_t>(method)].weights[3] = 0;
            check(PMXToModelData::to_model_data("valid.pmx", data, nullptr).is_ok(),
                "unused missing fourth influence accepted");
        }
        auto data = deformation_fixture();
        data.vertices[static_cast<std::size_t>(PMXDeformType::SDEF)].sdef_center[0] =
            std::numeric_limits<float>::quiet_NaN();
        check(PMXToModelData::to_model_data("invalid.pmx", data, nullptr).is_err(),
            "nonfinite spherical parameters rejected");
    }
} // namespace

void pmx_deformation_tests() {
    mixed_deformation_test();
    malformed_deformation_test();
}
