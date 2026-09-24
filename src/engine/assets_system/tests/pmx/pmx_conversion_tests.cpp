#include <assets_system/model/model_loader/pmx/pmx_to_model_data.h>
#include <cstdlib>
#include <iostream>

using namespace enishi;
using namespace enishi::assets_system;

namespace {
    constexpr std::uint8_t BDEF2 = 1;
    constexpr std::uint8_t BDEF4 = 2;
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

    std::int32_t morph_offset_index(std::uint8_t kind) {
        if (kind == 0) {
            return 1;
        }
        if (kind == 9) {
            return 9;
        }
        if (kind == 8) {
            return -1;
        }
        return 0;
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
        data.additional_uv_count = 0;
        data.vertices.resize(3);
        for (auto& vertex : data.vertices) {
            vertex.deform_type = BDEF2;
            vertex.bones = {0, 1, -1, -1};
            vertex.weights = {0.25f, 0.75f, 0, 0};
            vertex.edge_scale = 0.5f;
        }
        data.indices = {0, 1, 2};
        data.bones.resize(2);
        data.bones[0].name = "child";
        data.bones[0].parent = 1;
        data.bones[0].position = {4, 6, 8};
        data.bones[0].flags = 0x0020;
        data.bones[0].ik_target = 1;
        data.bones[0].ik_iterations = 5;
        data.bones[0].ik_angle = 0.25f;
        data.bones[0].ik_links.push_back({1, 0, {}, {}});
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
        {
            constexpr std::uint8_t spring_six_dof = 0;
            PMXJoint joint;
            joint.type = spring_six_dof;
            joint.body_a = 0;
            joint.body_b = 0;
            joint.translation_min = {-1, -2, -3};
            joint.translation_max = {1, 2, 3};
            joint.rotation_min = {-4, -5, -6};
            joint.rotation_max = {4, 5, 6};
            joint.translation_spring = {1, 2, 3};
            joint.rotation_spring = {4, 5, 6};
            data.joints.push_back(joint);
        }
        {
            constexpr std::uint8_t vertex_morph = 1;
            PMXMorph morph;
            morph.type = vertex_morph;
            morph.name = std::to_string(vertex_morph);
            PMXMorphOffset offset;
            offset.index = morph_offset_index(vertex_morph);
            offset.weight = 0.7f;
            offset.translation = {1, 2, 3};
            offset.rotation = {0, 0, 0, 1};
            offset.uv = {4, 3, 2, 1};
            offset.operation = 1;
            morph.offsets.push_back(offset);
            data.morphs.push_back(morph);
        }
        return data;
    }
} // namespace

void pmx_conversion_value_tests() {
    auto data = model_fixture();
    TestTextureLoader textures;
    auto converted = PMXToModelData::to_model_data("models/model.pmx", data, &textures);
    if (converted.is_err()) {
        std::cerr << converted.unwrap_err().get_message() << '\n';
    }
    check(converted.is_ok(), "model conversion failed");
    const auto& model = *converted.unwrap();
    check(model.name == "test model" && model.vertices.size() == 3, "model identity and vertices");
    const auto& skin = std::get<types::Skinning>(model.vertices.front()[1]);
    check(skin.bone_weight.y == 0.75f && skin.bone_index.y == 1, "two-influence vertex layout");
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
    check(!ik.link_limits.front().enabled, "unrestricted IK link");
    const auto& morphs = addon<types::AddonMorphs>(model);
    check(morphs.vertices.size() == 1 && morphs.vertices.front().front().index == 0 &&
              morphs.vertices.front().front().offset.z == 3,
        "legacy vertex morph conversion");
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
    check(joints.size() == 1 && joints[0].spring_rotation.z == 6, "joint count and springs");
}

void pmx_conversion_error_tests() {
    auto data = model_fixture();
    TestTextureLoader textures;
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
    data.joints[0].type = 255;
    check(PMXToModelData::to_model_data("model.pmx", data, nullptr).is_err(),
        "invalid direct conversion input");
}

void pmx_unsupported_before_texture_test() {
    auto data = model_fixture();
    data.vertices.front().deform_type = BDEF4;
    TestTextureLoader textures;
    textures.fail = true;
    const auto result = PMXToModelData::to_model_data("unsupported.pmx", data, &textures);
    check(result.is_err() && result.unwrap_err().get_error() == AssetError::UnsupportedFeature &&
              textures.paths.empty(),
        "unsupported model must be rejected before texture loading");
    data = model_fixture();
    constexpr std::size_t over_runtime_bone_limit = 513;
    data.bones.resize(over_runtime_bone_limit);
    const auto oversized = PMXToModelData::to_model_data("oversized.pmx", data, &textures);
    check(oversized.is_err() &&
              oversized.unwrap_err().get_error() == AssetError::UnsupportedFeature &&
              textures.paths.empty(),
        "oversized bone palette must be rejected before texture loading");
}

void pmx_conversion_tests() {
    pmx_conversion_value_tests();
    pmx_conversion_error_tests();
    pmx_unsupported_before_texture_test();
}
