#include <assets_system/model/model_loader/pmd/pmd_morph_converter.h>
#include <assets_system/model/model_loader/pmd/pmd_to_model_data.h>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <limits>

using namespace enishi;

namespace {
    constexpr std::size_t MODEL_VERTICES = 8;
    constexpr std::uint32_t FIRST_VERTEX = 7;
    constexpr std::uint32_t SECOND_VERTEX = 2;

    void check(bool condition, const char* message) {
        if (!condition) {
            std::cerr << message << '\n';
            std::exit(EXIT_FAILURE);
        }
    }

    std::vector<assets_system::PMDMorph> fixture() {
        std::vector<assets_system::PMDMorph> morphs(2);
        morphs[0].vertices = {{FIRST_VERTEX, {1, 2, 3}}, {SECOND_VERTEX, {4, 5, 6}}};
        auto& expression = morphs[1];
        expression.skin_type = 1;
        std::memcpy(expression.name, "smile", sizeof("smile"));
        expression.vertices = {{1, {0, 1, 0}}, {0, {1, 0, 0}}};
        return morphs;
    }

    void bone_hierarchy_test() {
        assets_system::PMDData source{};
        constexpr std::size_t CHILD = 0;
        constexpr std::size_t LEAF = 1;
        constexpr std::size_t ROOT = 2;
        source.bones.resize(3);
        source.bones[CHILD].parent_index = ROOT;
        source.bones[CHILD].position[0] = 3;
        source.bones[LEAF].parent_index = CHILD;
        source.bones[LEAF].position[0] = 5;
        source.bones[ROOT].parent_index = std::numeric_limits<std::uint16_t>::max();
        source.bones[ROOT].position[0] = 1;
        std::memcpy(source.bones[CHILD].name, "child", sizeof("child"));
        const auto result =
            assets_system::PMDToModelData::to_model_data("test.pmd", source, nullptr);
        check(result.is_ok(), "PMD without expressions converts");
        const auto& bones = std::get<types::AddonBones>(result.unwrap()->addons.front());
        check(bones[CHILD].name == "child", "common bone preserves unpadded name");
        check(bones[ROOT].bone_node.children == std::vector<types::BoneIndex>{CHILD} &&
                  bones[CHILD].bone_node.children == std::vector<types::BoneIndex>{LEAF} &&
                  bones[LEAF].bone_node.children.empty(),
            "children belong to the parent node");
        check(bones[CHILD].bind_bone.local[3].x == 2 && bones[LEAF].bind_bone.local[3].x == 2,
            "local translation is relative to parent model position");
        for (std::size_t index = 0; index < bones.size(); ++index) {
            const auto& bind = bones[index].bind_bone;
            check(glm::determinant(bind.local) == 1, "local matrix is invertible");
            check(bind.global[3].x == source.bones[index].position[0], "global bind position");
            check(bind.global * bind.global_inverse == glm::mat4(1),
                "inverse bind cancels rest pose");
        }
    }
} // namespace

int main() {
    bone_hierarchy_test();
    const auto empty = assets_system::convert_pmd_morphs({}, MODEL_VERTICES);
    check(empty.is_ok() && empty.unwrap().targets.targets.empty(), "zero morphs are valid");
    auto source = fixture();
    const auto result = assets_system::convert_pmd_morphs(source, MODEL_VERTICES);
    check(result.is_ok(), "PMD conversion");
    const auto& converted = result.unwrap();
    check(converted.targets.targets.size() == 1, "base is not an animated morph");
    const auto& target = converted.targets.targets.front();
    check(target.name == "smile", "fixed-width name must exclude null padding");
    const auto& first = std::get<types::VertexMorphOffset>(target.offsets[0]);
    const auto& second = std::get<types::VertexMorphOffset>(target.offsets[1]);
    check(first.vertex == SECOND_VERTEX && second.vertex == FIRST_VERTEX,
        "base table indices must resolve to model vertices");
    check(first.translation == glm::vec3(0, 1, 0), "offset is relative, not an absolute position");
    check(converted.legacy.vertices.front()[0].index == 1 &&
              converted.legacy.base_vertices[1].index == SECOND_VERTEX,
        "legacy base-table references must remain compatible");
    source[1].vertices[0].index = static_cast<std::uint32_t>(source[0].vertices.size());
    check(assets_system::convert_pmd_morphs(source, MODEL_VERTICES).is_err(),
        "invalid base reference");
    source = fixture();
    source[0].vertices[0].index = MODEL_VERTICES;
    check(assets_system::convert_pmd_morphs(source, MODEL_VERTICES).is_err(),
        "invalid model reference");
    source = fixture();
    source[1].vertices[0].position[0] = std::numeric_limits<float>::quiet_NaN();
    check(
        assets_system::convert_pmd_morphs(source, MODEL_VERTICES).is_err(), "invalid displacement");
    source = fixture();
    source[0].skin_type = 1;
    check(assets_system::convert_pmd_morphs(source, MODEL_VERTICES).is_err(), "missing base morph");
    std::cout << "PMD morph conversion tests passed\n";
}
