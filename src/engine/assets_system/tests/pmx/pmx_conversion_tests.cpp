#include <assets_system/model/model_loader/pmx/pmx_to_model_data.h>
#include <cstdlib>
#include <iostream>

using namespace enishi;
using namespace enishi::assets_system;

namespace {
    void check(bool condition, const char* message) {
        if (!condition) {
            std::cerr << message << '\n';
            std::exit(1);
        }
    }

    template <typename T> const T& addon(const types::ModelData& model) {
        for (const auto& value : model.addons) {
            if (const auto found = std::get_if<T>(&value)) {
                return *found;
            }
        }
        std::cerr << "missing model addon\n";
        std::exit(1);
    }

    class TestTextureLoader : public IAssetLoader {
      public:
        std::vector<std::filesystem::path> paths;
        bool fail{}, wrong_type{};
        foundation::Result<types::AssetData, AssetError> load(
            const std::filesystem::path& path) noexcept override {
            this->paths.push_back(path);
            if (this->fail) {
                return foundation::Error(AssetError::NotFound, "texture-test-error");
            }
            if (this->wrong_type) {
                return types::AssetData{std::make_shared<types::ModelData>()};
            }
            return types::AssetData{std::make_shared<types::TextureData>()};
        }
        std::vector<foundation::UTF8> get_supported_extension() const noexcept override {
            return {".png"};
        }
        types::AssetKind get_target_asset_type() const noexcept override {
            return types::AssetKind::Texture;
        }
    };

    PMXData model_fixture() {
        PMXData data;
        data.version = 2.1f;
        data.name = "test model";
        data.additional_uv_count = 1;
        data.vertices.resize(3);
        for (auto& vertex : data.vertices) {
            vertex.deform_type = 2;
            vertex.bones = {0, 1, 0, 1};
            vertex.weights = {0.1f, 0.2f, 0.3f, 0.4f};
            vertex.additional_uvs.push_back({1, 2, 3, 4});
            vertex.edge_scale = 0.5f;
        }
        data.vertices[1].deform_type = 4;
        data.indices = {0, 1, 2};
        data.bones.resize(2);
        data.bones[0].name = "child";
        data.bones[0].parent = 1;
        data.bones[0].position = {4, 6, 8};
        data.bones[0].flags = 0x0020;
        data.bones[0].ik_target = 1;
        data.bones[0].ik_iterations = 5;
        data.bones[0].ik_angle = 0.25f;
        data.bones[0].ik_links.push_back({1, 1, {-1, -2, -3}, {1, 2, 3}});
        data.bones[1].name = "root";
        data.bones[1].position = {1, 2, 3};
        data.textures = {"textures\\..\\色.png"};
        PMXMaterial material;
        material.name = "material";
        material.index_count = 3;
        material.diffuse = {0.2f, 0.4f, 0.6f, 1};
        material.texture = 0;
        material.sphere_texture = 0;
        material.sphere_mode = 2;
        material.flags = 0x1F;
        data.materials.push_back(material);
        PMXRigidBody body;
        body.name = "body";
        body.bone = 0;
        body.mode = 1;
        body.shape = 2;
        body.size = {2, 3, 4};
        body.mass = 5;
        body.position = {7, 8, 9};
        body.non_collision_mask = 0x0003;
        data.rigid_bodies.push_back(body);
        for (std::uint8_t kind = 0; kind < 6; ++kind) {
            PMXJoint joint;
            joint.type = kind;
            joint.body_a = 0;
            joint.translation_min = {-1, -2, -3};
            joint.translation_max = {1, 2, 3};
            joint.rotation_min = {-4, -5, -6};
            joint.rotation_max = {4, 5, 6};
            joint.translation_spring = {1, 2, 3};
            joint.rotation_spring = {4, 5, 6};
            data.joints.push_back(joint);
        }
        for (std::uint8_t kind = 0; kind < 11; ++kind) {
            PMXMorph morph;
            morph.type = kind;
            morph.name = std::to_string(kind);
            PMXMorphOffset offset;
            offset.index = kind == 0 ? 1 : kind == 9 ? 9 : kind == 8 ? -1 : 0;
            offset.weight = 0.7f;
            offset.translation = {1, 2, 3};
            offset.rotation = {0, 0, 0, 1};
            offset.uv = {4, 3, 2, 1};
            offset.operation = 1;
            morph.offsets.push_back(offset);
            data.morphs.push_back(morph);
        }
        PMXSoftBody soft;
        soft.material = 0;
        soft.mass = 10;
        soft.config[2] = 0.5f;
        soft.iterations = {1, 2, 3, 4};
        soft.anchors.push_back({0, 1, 0});
        soft.pinned_vertices.push_back(2);
        data.soft_bodies.push_back(soft);
        return data;
    }
} // namespace

void pmx_conversion_tests() {
    auto data = model_fixture();
    TestTextureLoader textures;
    auto converted = PMXToModelData::to_model_data("models/model.pmx", data, &textures);
    if (converted.is_err()) {
        std::cerr << converted.unwrap_err().get_message() << '\n';
    }
    check(converted.is_ok(), "model conversion failed");
    const auto& model = *converted.unwrap();
    check(model.name == "test model" && model.vertices.size() == 3, "model identity and vertices");
    const auto& skin = std::get<types::Skinning4>(model.vertices[0][1]);
    check(skin.bone_weight.w == 0.4f && skin.bone_index.w == 1, "four influences preserved");
    check(
        model.skinning_methods[1] == types::SkinningMethod::DualQuaternion, "dual quaternion mode");
    check(model.additional_uv_channels[0][2].w == 4, "additional UV channel");
    const auto& bones = addon<types::AddonBones>(model);
    check(bones[0].bind_bone.local[3] == glm::vec4(3, 4, 5, 1), "child local translation");
    check(
        bones[0].bind_bone.global[3] == glm::vec4(4, 6, 8, 1), "forward parent global translation");
    check(bones[0].bind_bone.global_inverse[3] == glm::vec4(-4, -6, -8, 1),
        "inverse bind translation");
    check(bones[1].bone_node.children == std::vector<types::BoneIndex>{0}, "parent children list");
    const auto& iks = addon<types::AddonIKs>(model);
    check(iks.size() == 1, "IK count");
    const auto& ik = std::get<types::CCDIK>(iks[0].method);
    check(ik.iterations == 5 && ik.ik_bone == 0 && ik.target == 1, "IK metadata");
    check(ik.link_limits[0].lower.y == -2, "per-link IK limit");
    const auto& morphs = addon<types::AddonMorphTargets>(model).targets;
    check(morphs.size() == 11, "all morph types preserved");
    check(std::get<types::VertexMorphOffset>(morphs[1].offsets[0]).translation.z == 3,
        "direct vertex morph index");
    check(std::get<types::BoneMorphOffset>(morphs[2].offsets[0]).rotation.w == 1,
        "quaternion component order");
    check(std::get<types::UVMorphOffset>(morphs[7].offsets[0]).channel == 4, "UV morph channel");
    check(std::get<types::MaterialMorphOffset>(morphs[8].offsets[0]).material == UINT32_MAX,
        "all-material sentinel");
    check(morphs[9].weight_mode == types::MorphWeightMode::DiscreteSelection,
        "discrete morph selection");
    check(std::get<types::ImpulseMorphOffset>(morphs[10].offsets[0]).local_space, "impulse space");
    const auto& vertex_morphs = addon<types::AddonMorphs>(model);
    check(vertex_morphs.vertices.size() == 11 && vertex_morphs.vertices[1][0].index == 0,
        "legacy morph mapping");
    check(textures.paths.size() == 1 && model.textures.size() == 1, "texture deduplication");
    check(textures.paths[0] == std::filesystem::path(u8"models/色.png"),
        "UTF-8 texture path normalization");
    check(model.materials[0].first_offset == 0 && model.materials[0].count == 3, "material range");
    check(model.materials[0].double_sided &&
              model.materials[0].textures[1].blend == types::MaterialTexture::Blend::Add,
        "material flags and environment blend");
    const auto& bodies = addon<types::AddonRigidBodies>(model);
    check(bodies.size() == 1 && bodies[0].mass == 5 && bodies[0].group_mask == 0xFFFC,
        "rigid body conversion");
    check(bodies[0].position == glm::vec3(3, 2, 1), "rigid body bone-relative offset");
    const auto& joints = addon<types::AddonPhysicsJoints>(model);
    check(joints.size() == 6 && joints[0].spring_rotation.z == 6, "joint count and springs");
    check(joints[3].kind == types::JointKind::ConeTwist && joints[3].angular_span.z == -6,
        "cone twist mapping");
    check(joints[4].linear_motor.velocity == 2 && joints[5].angular_motor.max_force == 6,
        "joint motors");
    const auto& soft = addon<types::AddonSoftBodies>(model).bodies[0];
    check(soft.settings.drag == 0.5f && soft.settings.cluster_iterations == 4,
        "soft body solver settings");
    check(
        soft.anchors[0].vertex == 1 && soft.pinned_vertices[0] == 2, "soft body anchors and pins");

    textures.fail = true;
    auto failed = PMXToModelData::to_model_data("models/model.pmx", data, &textures);
    check(failed.is_err() &&
              failed.unwrap_err().get_message().find("texture-test-error") != std::string::npos,
        "texture error propagation");
    textures.fail = false;
    textures.wrong_type = true;
    check(PMXToModelData::to_model_data("model.pmx", data, &textures).is_err(),
        "wrong texture asset type");
    check(PMXToModelData::to_model_data("model.pmx", data, nullptr).is_err(),
        "missing texture loader");
    data.materials[0].texture = -1;
    data.materials[0].sphere_texture = -1;
    check(PMXToModelData::to_model_data("model.pmx", data, nullptr).is_ok(),
        "untextured model conversion");
    data.bones.resize(65537);
    data.vertices[0].bones[0] = 65535;
    auto large = PMXToModelData::to_model_data("model.pmx", data, nullptr);
    check(large.is_ok() &&
              std::get<types::Skinning4>(large.unwrap()->vertices[0][1]).bone_index.x == 65535,
        "wide bone index must not become the root sentinel");
    data.joints[0].type = 255;
    check(PMXToModelData::to_model_data("model.pmx", data, nullptr).is_err(),
        "invalid direct conversion input");

    PMXData narrow;
    narrow.version = 2.0f;
    narrow.bones.resize(1);
    PMXVertex vertex;
    vertex.deform_type = 3;
    vertex.bones = {0, -1, -1, -1};
    vertex.weights = {0.25f, 0.75f, 0, 0};
    narrow.vertices.push_back(vertex);
    auto linear = PMXToModelData::to_model_data("model.pmx", narrow, nullptr);
    check(linear.is_ok() &&
              std::get<types::Skinning>(linear.unwrap()->vertices[0][1]).bone_weight.y == 0.75f,
        "two-influence compatibility and spherical fallback");
    data = model_fixture();
    data.materials[0].texture = -1;
    data.materials[0].sphere_texture = -1;
    data.morphs[10].offsets[0].translation = {};
    auto stopped = PMXToModelData::to_model_data("model.pmx", data, nullptr);
    check(stopped.is_ok() &&
              std::get<types::ImpulseMorphOffset>(
                  addon<types::AddonMorphTargets>(*stopped.unwrap()).targets[10].offsets[0])
                  .reset_velocity,
        "zero impulse becomes a velocity reset");
}
