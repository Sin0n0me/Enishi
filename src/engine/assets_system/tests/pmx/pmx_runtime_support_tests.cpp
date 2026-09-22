#include <assets_system/model/model_loader/pmx/pmx_model_loader.h>
#include <assets_system/model/model_loader/pmx/pmx_to_model_data.h>
#include <cstdlib>
#include <iostream>

using namespace enishi;
using namespace enishi::assets_system;

namespace {
    constexpr float PMX_VERSION = 2.1f;
    constexpr std::int32_t NO_BONE = -1;
    constexpr std::int32_t ROOT_BONE = 0;
    constexpr std::int32_t ATTACHED_BODY = 1;

    void check(bool condition, const char* message) {
        if (!condition) {
            std::cerr << message << '\n';
            std::exit(EXIT_FAILURE);
        }
    }

    PMXData supported_model() {
        PMXData data;
        data.version = PMX_VERSION;
        data.bones.emplace_back();
        auto& vertex = data.vertices.emplace_back();
        vertex.bones = {ROOT_BONE, ROOT_BONE, NO_BONE, NO_BONE};
        vertex.weights.front() = 1.0f;
        return data;
    }

    void unbound_rigid_body_tests() {
        auto data = supported_model();
        auto& unbound = data.rigid_bodies.emplace_back();
        unbound.bone = NO_BONE;
        auto& attached = data.rigid_bodies.emplace_back();
        attached.bone = ROOT_BONE;
        auto& joint = data.joints.emplace_back();
        joint.body_a = ATTACHED_BODY;
        joint.body_b = ATTACHED_BODY;
        check(PMXModelLoader::validate(data).is_ok(), "unbound body is valid PMX source data");
        const auto result = PMXToModelData::to_model_data("unbound.pmx", data, nullptr);
        check(result.is_err() &&
                  result.unwrap_err().get_error() == AssetError::UnsupportedFeature &&
                  result.unwrap_err().get_message().contains("without a related bone"),
            "unbound body must fail before joint indices can be compacted");
        data.rigid_bodies.front().bone = ROOT_BONE;
        check(PMXToModelData::to_model_data("bound.pmx", data, nullptr).is_ok(),
            "bound rigid bodies must remain supported");
    }
} // namespace

void pmx_runtime_support_tests() {
    unbound_rigid_body_tests();
}
