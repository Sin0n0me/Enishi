#include <cstdlib>
#include <cstring>
#include <iostream>
#include <renderer/common/converter/model_to_mesh.h>

using namespace enishi;

namespace {
    void check(bool value, const char* message) {
        if (!value) {
            std::cerr << message << '\n';
            std::exit(EXIT_FAILURE);
        }
    }
} // namespace

int main() {
    types::ModelData model;
    model.vertices = {{types::VertexPosition{{0, 0, 0}}}};
    model.indices = std::vector<std::uint32_t>{0, 0};
    model.materials.resize(2);
    for (auto& material : model.materials) {
        material.name = "Material";
        material.count = 1;
        material.instance_count = 1;
    }
    model.materials[0].variants = {types::Diffuse{{1, 0, 0, 1}}};
    model.materials[1].variants = {types::Diffuse{{0, 1, 0, 1}}};
    renderer::ModelToMesh::MeshConfig config;
    config.use_camera = false;
    auto converted = renderer::ModelToMesh::to_mesh_data(model, std::move(config));
    if (converted.is_err()) {
        std::cerr << converted.unwrap_err().get_message() << '\n';
    }
    check(converted.is_ok(), "convert two materials");
    const auto& mesh = converted.unwrap();
    check(!mesh.uniforms.contains("Material"), "material uniforms must not be shared across draws");
    check(mesh.materials.size() == 2, "retain draw order and material count");
    for (std::size_t index = 0; index < mesh.materials.size(); ++index) {
        const auto& data = mesh.materials[index].uniforms->at("Material").get_render_data();
        glm::vec4 color;
        std::memcpy(&color, data.raw_data(), sizeof(color));
        check(color == std::get<types::Diffuse>(model.materials[index].variants[0]).color,
            "same-named materials retain independent uniform contents");
        check(data.byte_width() == sizeof(color), "aligned material must not gain an extra block");
    }
    std::cout << "Material binding tests passed\n";
}
