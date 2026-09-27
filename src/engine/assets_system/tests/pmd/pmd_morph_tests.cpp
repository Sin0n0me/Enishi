#include <assets_system/model/model_loader/pmd/pmd_morph_converter.h>
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
} // namespace

int main() {
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
