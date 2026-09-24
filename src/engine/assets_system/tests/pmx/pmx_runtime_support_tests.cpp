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
    constexpr std::uint8_t BDEF4 = 2;
    constexpr std::uint8_t SDEF = 3;
    constexpr std::uint8_t QDEF = 4;
    constexpr std::uint8_t ONE_ADDITIONAL_UV = 1;
    constexpr std::uint8_t GROUP_MORPH = 0;
    constexpr std::uint8_t BONE_MORPH = 2;
    constexpr std::uint8_t UV_MORPH = 3;
    constexpr std::uint8_t FIRST_ADDITIONAL_UV_MORPH = 4;
    constexpr std::uint8_t SECOND_ADDITIONAL_UV_MORPH = 5;
    constexpr std::uint8_t THIRD_ADDITIONAL_UV_MORPH = 6;
    constexpr std::uint8_t FOURTH_ADDITIONAL_UV_MORPH = 7;
    constexpr std::uint8_t MATERIAL_MORPH = 8;
    constexpr std::uint8_t FLIP_MORPH = 9;
    constexpr std::uint8_t IMPULSE_MORPH = 10;
    constexpr std::uint16_t BONE_IK = 0x0020;
    constexpr std::uint16_t INHERIT_ROTATION = 0x0100;
    constexpr std::uint16_t INHERIT_TRANSLATION = 0x0200;
    constexpr std::uint16_t FIXED_AXIS = 0x0400;
    constexpr std::uint16_t LOCAL_AXES = 0x0800;
    constexpr std::uint16_t AFTER_PHYSICS = 0x1000;
    constexpr std::uint16_t EXTERNAL_PARENT = 0x2000;
    constexpr std::uint8_t LIMIT_ENABLED = 1;
    constexpr std::uint8_t MATERIAL_VERTEX_COLOR = 0x20;
    constexpr std::uint8_t MATERIAL_POINTS = 0x40;
    constexpr std::uint8_t MATERIAL_LINES = 0x80;
    constexpr std::uint8_t SPHERE_SUB_TEXTURE = 3;
    constexpr std::uint8_t SIX_DOF_JOINT = 1;
    constexpr std::uint8_t POINT_JOINT = 2;
    constexpr std::uint8_t CONE_JOINT = 3;
    constexpr std::uint8_t SLIDER_JOINT = 4;
    constexpr std::uint8_t HINGE_JOINT = 5;

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

    void expect_unsupported(const PMXData& data, const char* feature) {
        check(PMXModelLoader::validate(data).is_ok(),
            "runtime limitation must not reject valid source data");
        const auto result = PMXToModelData::to_model_data("unsupported.pmx", data, nullptr);
        check(result.is_err() &&
                  result.unwrap_err().get_error() == AssetError::UnsupportedFeature &&
                  result.unwrap_err().get_message().contains(feature),
            feature);
    }

    void rendering_support_tests() {
        for (const auto deform : {BDEF4, SDEF, QDEF}) {
            auto data = supported_model();
            data.vertices.front().deform_type = deform;
            expect_unsupported(data, "skinning");
        }
        auto data = supported_model();
        data.additional_uv_count = ONE_ADDITIONAL_UV;
        data.vertices.front().additional_uvs.emplace_back();
        expect_unsupported(data, "additional UV channels");
        for (const auto flag : {MATERIAL_VERTEX_COLOR, MATERIAL_POINTS, MATERIAL_LINES}) {
            data = supported_model();
            data.materials.emplace_back().flags = flag;
            expect_unsupported(data, "vertex colors or point/line materials");
        }
        data = supported_model();
        data.materials.emplace_back().sphere_mode = SPHERE_SUB_TEXTURE;
        expect_unsupported(data, "sphere sub-textures");
    }

    void missing_influence_tests() {
        auto data = supported_model();
        data.vertices.front().bones[1] = NO_BONE;
        const auto converted = PMXToModelData::to_model_data("single-bone.pmx", data, nullptr);
        constexpr std::size_t skinning_attribute = 1;
        check(converted.is_ok() && std::get<types::Skinning>(
                                       converted.unwrap()->vertices.front()[skinning_attribute])
                                           .bone_index.y == ROOT_BONE,
            "BDEF1 unused influence must not index a missing shader matrix");
        data.vertices.front().bones.front() = NO_BONE;
        expect_unsupported(data, "weighted influences without bones");
        data.vertices.front().weights.front() = 0.0f;
        data.bones.clear();
        expect_unsupported(data, "models without bones");
    }

    void animation_support_tests() {
        for (const auto flag : {INHERIT_ROTATION,
                 INHERIT_TRANSLATION,
                 FIXED_AXIS,
                 LOCAL_AXES,
                 AFTER_PHYSICS,
                 EXTERNAL_PARENT}) {
            auto data = supported_model();
            data.bones.front().flags = flag;
            expect_unsupported(data, "bone inheritance, axes, or external/physics transforms");
        }
        auto data = supported_model();
        data.bones.front().layer = 1;
        expect_unsupported(data, "bone evaluation layers");
        data = supported_model();
        auto& bone = data.bones.front();
        bone.flags = BONE_IK;
        bone.ik_target = ROOT_BONE;
        bone.ik_links.push_back({ROOT_BONE, LIMIT_ENABLED});
        expect_unsupported(data, "per-link IK limits");
        for (const auto type : {GROUP_MORPH,
                 BONE_MORPH,
                 UV_MORPH,
                 FIRST_ADDITIONAL_UV_MORPH,
                 SECOND_ADDITIONAL_UV_MORPH,
                 THIRD_ADDITIONAL_UV_MORPH,
                 FOURTH_ADDITIONAL_UV_MORPH,
                 MATERIAL_MORPH,
                 FLIP_MORPH,
                 IMPULSE_MORPH}) {
            data = supported_model();
            data.morphs.emplace_back().type = type;
            expect_unsupported(data, "non-vertex morphs");
        }
    }

    void physics_support_tests() {
        for (const auto type :
            {SIX_DOF_JOINT, POINT_JOINT, CONE_JOINT, SLIDER_JOINT, HINGE_JOINT}) {
            auto data = supported_model();
            data.joints.emplace_back().type = type;
            expect_unsupported(data, "non-spring-six-DOF joints");
        }
        auto data = supported_model();
        data.joints.emplace_back();
        expect_unsupported(data, "joints without two rigid bodies");
        data = supported_model();
        data.materials.emplace_back();
        data.soft_bodies.emplace_back().material = 0;
        expect_unsupported(data, "soft bodies");
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
    rendering_support_tests();
    missing_influence_tests();
    animation_support_tests();
    physics_support_tests();
}
