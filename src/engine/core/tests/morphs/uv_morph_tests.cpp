#include <core/system/animation/uv_morph.h>
#include <core/system/render/uv_morph_upload.h>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <renderer/common/converter/skinned_vertices.h>
#include <renderer/common/vertex_buffer_updater.h>

using namespace enishi;

namespace {
    void check(bool value, const char* message) {
        if (!value) {
            std::cerr << message << '\n';
            std::exit(EXIT_FAILURE);
        }
    }
} // namespace

void uv_morph_tests() {
    core::UVChannels base(5, std::vector<glm::vec4>(1, glm::vec4(1)));
    std::vector<types::MorphTarget> targets(2);
    targets[0].offsets = {
        types::UVMorphOffset{0, 0, {2, -2, 0, 0}}, types::UVMorphOffset{0, 4, {1, 2, 3, 4}}};
    targets[1].offsets = {types::MorphWeightOffset{0, 0.5f}};
    const std::vector<float> weights{0, 1};
    const auto result = core::evaluate_uv_morphs(base, targets, weights);
    check(result.is_ok() && result.unwrap()[0][0] == glm::vec4(2, 0, 1, 1) &&
              result.unwrap()[4][0] == glm::vec4(1.5f, 2, 2.5f, 3),
        "group weights update all UV components");
    check(core::evaluate_uv_morphs(base, targets, std::vector<float>{0, 0}).unwrap() == base,
        "UV weights reset without accumulating");
    std::vector<renderer::SkinnedVertex> vertices(1);
    vertices[0].position = {7, 8, 9};
    bool uploaded = false;
    renderer::VertexBufferUpdater updater(
        types::OwnedRenderData(vertices), [&](const types::RenderData& data) {
            renderer::SkinnedVertex vertex;
            std::memcpy(&vertex, data.raw_data(), sizeof(vertex));
            check(vertex.position == vertices[0].position && vertex.uv == glm::vec2(2, 0),
                "UV upload preserves position and updates primary UV");
            check(vertex.additional_uvs.back() == glm::vec4(1.5f, 2, 2.5f, 3),
                "additional UV reaches upload callback");
            uploaded = true;
        });
    auto streams = renderer::skinned_uv_streams({});
    check(core::stage_vertex_uvs(updater, streams, result.unwrap()).is_ok() && !uploaded,
        "UV changes stage before the combined vertex upload");
    updater.on_update();
    check(uploaded, "upload staged UVs");
    streams.front().offset = sizeof(renderer::SkinnedVertex);
    check(core::stage_vertex_uvs(updater, streams, result.unwrap()).is_err(),
        "reject invalid UV layout");
    targets[0].offsets = {types::UVMorphOffset{1, 0, {1, 0, 0, 0}}};
    check(core::evaluate_uv_morphs(base, targets, weights).is_err(), "reject invalid UV vertex");
}
