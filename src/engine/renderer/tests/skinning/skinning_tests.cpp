#include <array>
#include <cmath>
#include <cstring>
#include <d3d11.h>
#include <d3d11shader.h>
#include <d3dcompiler.h>
#include <fstream>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <iostream>
#include <iterator>
#include <renderer/common/converter/skinned_vertices.h>
#include <wrl/client.h>

using namespace enishi;
using Microsoft::WRL::ComPtr;

namespace {
    constexpr std::size_t BONE_CAPACITY = 512;
    constexpr float TOLERANCE = 0.0001f;
    using Palette = std::array<glm::mat4, BONE_CAPACITY>;
    struct DeformedVertex {
        glm::vec3 position;
        glm::vec3 normal;
    };

    void check(bool condition, const char* message) {
        if (!condition) {
            std::cerr << message << '\n';
            std::exit(EXIT_FAILURE);
        }
    }
    void succeeded(HRESULT result) {
        check(SUCCEEDED(result), "D3D test operation failed");
    }

    std::string read_shader(const char* relative) {
        std::ifstream input(std::string(SHADER_ROOT) + relative);
        check(input.is_open(), "shader source missing");
        return {std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
    }

    class ShaderRunner {
        ComPtr<ID3D11Device> device;
        ComPtr<ID3D11DeviceContext> context;
        ComPtr<ID3D11ComputeShader> shader;
        ComPtr<ID3D11Buffer> palette;
        ComPtr<ID3D11Buffer> input;
        ComPtr<ID3D11Buffer> output;
        ComPtr<ID3D11Buffer> readback;
        ComPtr<ID3D11ShaderResourceView> input_view;
        ComPtr<ID3D11UnorderedAccessView> output_view;

        ComPtr<ID3D11Buffer> buffer(UINT size, UINT bind, UINT stride = 0) {
            D3D11_BUFFER_DESC desc{};
            desc.ByteWidth = size;
            desc.BindFlags = bind;
            desc.Usage = D3D11_USAGE_DEFAULT;
            desc.StructureByteStride = stride;
            if (stride != 0) {
                desc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
            }
            ComPtr<ID3D11Buffer> result;
            succeeded(this->device->CreateBuffer(&desc, nullptr, &result));
            return result;
        }

      public:
        ShaderRunner() {
            succeeded(D3D11CreateDevice(nullptr,
                D3D_DRIVER_TYPE_WARP,
                nullptr,
                0,
                nullptr,
                0,
                D3D11_SDK_VERSION,
                &this->device,
                nullptr,
                &this->context));
            const auto source = read_shader("/hlsl/common/input_vs_model.hlsl") +
                                read_shader("/hlsl/common/constant_buffer/bones.hlsl") + R"(
StructuredBuffer<VSInput> vertices : register(t0);
struct Output { float3 position; float3 normal; };
RWStructuredBuffer<Output> results : register(u0);
[numthreads(1,1,1)] void main() {
    VSInput v = vertices[0];
    DeformedVertex d = deform_vertex(v.position, v.normal, v.bones, v.weights,
        v.skinning_method, v.blend_center, v.blend_anchor0, v.blend_anchor1);
    Output o; o.position = d.position; o.normal = d.normal; results[0] = o;
})";
            ComPtr<ID3DBlob> code;
            ComPtr<ID3DBlob> errors;
            const auto compiled = D3DCompile(source.data(),
                source.size(),
                nullptr,
                nullptr,
                nullptr,
                "main",
                "cs_5_0",
                D3DCOMPILE_WARNINGS_ARE_ERRORS,
                0,
                &code,
                &errors);
            if (FAILED(compiled) && errors != nullptr) {
                std::cerr << static_cast<const char*>(errors->GetBufferPointer());
            }
            succeeded(compiled);
            succeeded(this->device->CreateComputeShader(
                code->GetBufferPointer(), code->GetBufferSize(), nullptr, &this->shader));
            this->palette = this->buffer(sizeof(Palette), D3D11_BIND_CONSTANT_BUFFER);
            this->input = this->buffer(sizeof(renderer::SkinnedVertex),
                D3D11_BIND_SHADER_RESOURCE,
                sizeof(renderer::SkinnedVertex));
            this->output = this->buffer(
                sizeof(DeformedVertex), D3D11_BIND_UNORDERED_ACCESS, sizeof(DeformedVertex));
            succeeded(this->device->CreateShaderResourceView(
                this->input.Get(), nullptr, &this->input_view));
            succeeded(this->device->CreateUnorderedAccessView(
                this->output.Get(), nullptr, &this->output_view));
            D3D11_BUFFER_DESC desc{};
            desc.ByteWidth = sizeof(DeformedVertex);
            desc.Usage = D3D11_USAGE_STAGING;
            desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
            succeeded(this->device->CreateBuffer(&desc, nullptr, &this->readback));
        }

        DeformedVertex run(const renderer::SkinnedVertex& vertex, const Palette& bones) {
            this->context->UpdateSubresource(this->palette.Get(), 0, nullptr, bones.data(), 0, 0);
            this->context->UpdateSubresource(this->input.Get(), 0, nullptr, &vertex, 0, 0);
            this->context->CSSetShader(this->shader.Get(), nullptr, 0);
            this->context->CSSetConstantBuffers(0, 1, this->palette.GetAddressOf());
            this->context->CSSetShaderResources(0, 1, this->input_view.GetAddressOf());
            this->context->CSSetUnorderedAccessViews(
                0, 1, this->output_view.GetAddressOf(), nullptr);
            this->context->Dispatch(1, 1, 1);
            this->context->CopyResource(this->readback.Get(), this->output.Get());
            D3D11_MAPPED_SUBRESOURCE mapped{};
            succeeded(this->context->Map(this->readback.Get(), 0, D3D11_MAP_READ, 0, &mapped));
            DeformedVertex result;
            std::memcpy(&result, mapped.pData, sizeof(result));
            this->context->Unmap(this->readback.Get(), 0);
            return result;
        }
    };

    void check_near(const glm::vec3& actual, const glm::vec3& expected, const char* message) {
        if (!(glm::length(actual - expected) < TOLERANCE)) {
            std::cerr << "actual: " << actual.x << ',' << actual.y << ',' << actual.z << '\n';
            check(false, message);
        }
    }

    void deformation_tests(ShaderRunner& runner) {
        Palette bones;
        bones.fill(glm::mat4(1.0f));
        renderer::SkinnedVertex vertex;
        vertex.position = {2, 3, 4};
        vertex.normal = {1, 0, 0};
        vertex.bones = {0, 1, 2, 3};
        vertex.weights = {0.1f, 0.2f, 0.3f, 0.4f};
        for (std::uint32_t method = 0; method < 2; ++method) {
            vertex.method = method;
            check_near(runner.run(vertex, bones).position, vertex.position, "identity deformation");
            for (std::size_t bone = 0; bone < 4; ++bone) {
                bones[bone] =
                    glm::translate(glm::mat4(1), glm::vec3(static_cast<float>(bone), 0, 0));
            }
            check_near(runner.run(vertex, bones).position,
                vertex.position + glm::vec3(2, 0, 0),
                "all four translations must contribute");
            bones.fill(glm::mat4(1));
        }
        vertex.position = {1, 0, 0};
        vertex.weights = {0.5f, 0.5f, 0, 0};
        bones[1] = glm::rotate(glm::mat4(1), glm::half_pi<float>(), glm::vec3(0, 0, 1));
        vertex.method = 0;
        check_near(runner.run(vertex, bones).position, {0.5f, 0.5f, 0}, "linear blend chord");
        const auto half_rotation = glm::angleAxis(glm::quarter_pi<float>(), glm::vec3(0, 0, 1));
        for (std::uint32_t method = 1; method < 3; ++method) {
            vertex.method = method;
            const auto result = runner.run(vertex, bones);
            check_near(result.position, half_rotation * vertex.position, "quaternion blend arc");
            check_near(result.normal, half_rotation * vertex.normal, "quaternion blended normal");
        }
        vertex.method = 1;
        bones[0] = glm::rotate(glm::mat4(1), glm::radians(170.0f), glm::vec3(0, 0, 1));
        bones[1] = glm::rotate(glm::mat4(1), glm::radians(-170.0f), glm::vec3(0, 0, 1));
        check_near(
            runner.run(vertex, bones).position, {-1, 0, 0}, "antipodal quaternion correction");
        vertex.weights = {0, 1, 0, 0};
        for (const auto axis : {glm::vec3(1, 0, 0), glm::vec3(0, 1, 0), glm::vec3(0, 0, 1)}) {
            bones[1] = glm::rotate(glm::mat4(1), glm::pi<float>(), axis);
            check_near(runner.run(vertex, bones).position,
                glm::vec3(bones[1] * glm::vec4(vertex.position, 1)),
                "half-turn rotation and zero first influence");
        }
    }

    void spherical_anchor_tests(ShaderRunner& runner) {
        Palette bones;
        bones.fill(glm::mat4(1));
        renderer::SkinnedVertex vertex;
        vertex.method = 2;
        vertex.position = {4, 2, 1};
        vertex.normal = {0, 1, 0};
        vertex.bones = {0, 1, 0, 0};
        vertex.weights = {0.25f, 0.75f, 0, 0};
        vertex.center = {1, 2, 3};
        vertex.anchor0 = vertex.center + glm::vec3(3, 0, 0);
        vertex.anchor1 = vertex.center - glm::vec3(1, 0, 0);
        check_near(runner.run(vertex, bones).position,
            vertex.position,
            "spherical bind pose with anchors");
        const auto rotation = glm::angleAxis(glm::half_pi<float>(), glm::vec3(0, 0, 1));
        bones[1] = glm::translate(glm::mat4(1), glm::vec3(3, 4, 5)) * glm::mat4_cast(rotation);
        const auto blend = glm::slerp(glm::quat(1, 0, 0, 0), rotation, vertex.weights.y);
        const auto expected = blend * (vertex.position - vertex.center) +
                              vertex.anchor0 * vertex.weights.x +
                              glm::vec3(bones[1] * glm::vec4(vertex.anchor1, 1)) * vertex.weights.y;
        check_near(
            runner.run(vertex, bones).position, expected, "spherical corrected anchor deformation");
    }

    void packing_tests() {
        types::ModelData model;
        model.vertices.push_back({types::Vertex{{1, 2, 3}, {0, 1, 0}, {0, 0}},
            types::Skinning{{3, 4}, {0.25f, 0.75f}},
            types::EdgeFlag{0.5f}});
        auto result = renderer::make_skinned_vertices(model);
        check(result.is_ok(), "legacy skinning conversion");
        const auto& vertex = result.unwrap().front();
        check(vertex.bones == glm::uvec4(3, 4, 0, 0) &&
                  vertex.weights == glm::vec4(0.25f, 0.75f, 0, 0),
            "legacy PMD indices and weights preserved");
        model.vertices.front()[1] = types::Skinning4{{3, 4, 5, UINT32_MAX}, {0.2f, 0.3f, 0.5f, 0}};
        result = renderer::make_skinned_vertices(model);
        check(result.is_ok() && result.unwrap().front().bones == glm::uvec4(3, 4, 5, 0),
            "four-influence packing and missing zero-weight reference");
        model.skinning_methods = {types::SkinningMethod::SphericalBlend};
        check(
            renderer::make_skinned_vertices(model).is_err(), "missing spherical anchors rejected");
        model.spherical_blends.push_back({{1, 2, 3}, {4, 5, 6}, {7, 8, 9}});
        result = renderer::make_skinned_vertices(model);
        check(result.is_ok() && result.unwrap().front().method == 2 &&
                  result.unwrap().front().anchor1 == glm::vec3(7, 8, 9),
            "spherical GPU metadata");
        check(renderer::skinned_vertex_offset("BLEND_ANCHOR_B") ==
                  offsetof(renderer::SkinnedVertex, anchor1),
            "shadow and edge input layout offset");
    }
} // namespace

void shader_signature_tests() {
    for (const auto* file : {"vs_model.hlsl", "vs_model_edge.hlsl", "vs_shadow_map.hlsl"}) {
        const auto path = std::filesystem::path(SHADER_ROOT) / "hlsl" / file;
        ComPtr<ID3DBlob> code;
        ComPtr<ID3DBlob> errors;
        const auto result = D3DCompileFromFile(path.c_str(),
            nullptr,
            D3D_COMPILE_STANDARD_FILE_INCLUDE,
            "main",
            "vs_5_0",
            D3DCOMPILE_WARNINGS_ARE_ERRORS,
            0,
            &code,
            &errors);
        if (FAILED(result) && errors != nullptr) {
            std::cerr << static_cast<const char*>(errors->GetBufferPointer());
        }
        succeeded(result);
        ComPtr<ID3D11ShaderReflection> reflection;
        succeeded(D3DReflect(code->GetBufferPointer(),
            code->GetBufferSize(),
            IID_ID3D11ShaderReflection,
            reinterpret_cast<void**>(reflection.GetAddressOf())));
        D3D11_SHADER_DESC shader{};
        succeeded(reflection->GetDesc(&shader));
        for (UINT index = 0; index < shader.InputParameters; ++index) {
            D3D11_SIGNATURE_PARAMETER_DESC input{};
            succeeded(reflection->GetInputParameterDesc(index, &input));
            check(renderer::skinned_vertex_offset(input.SemanticName).has_value(),
                "all reflected model inputs must have explicit vertex offsets");
        }
    }
}

int main() {
    shader_signature_tests();
    packing_tests();
    ShaderRunner runner;
    deformation_tests(runner);
    spherical_anchor_tests(runner);
    std::cout << "Skinning packing and WARP shader tests passed\n";
}
