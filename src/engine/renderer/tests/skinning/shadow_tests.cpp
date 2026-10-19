#include <array>
#include <cmath>
#include <cstdlib>
#include <d3d11.h>
#include <d3d11shader.h>
#include <d3dcompiler.h>
#include <engine_types/renderer/uniform_buffer/camera.h>
#include <engine_types/renderer/uniform_buffer/material.h>
#include <engine_types/renderer/uniform_buffer/shadow.h>
#include <filesystem>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
#include <renderer/common/converter/skinned_vertices.h>
#include <renderer/directx/directx11/d3d11_converter.h>
#include <wrl/client.h>

using namespace enishi;
using Microsoft::WRL::ComPtr;

namespace {
    constexpr UINT SIZE = 64;
    constexpr UINT BONE_COUNT = 512;
    constexpr float TOLERANCE = 0.001f;
    void check(bool condition, const char* message) {
        if (!condition) {
            std::cerr << message << '\n';
            std::exit(EXIT_FAILURE);
        }
    }
    void succeeded(HRESULT result) {
        check(SUCCEEDED(result), "D3D shadow test operation failed");
    }
    ComPtr<ID3DBlob> compile(const char* file, const char* profile) {
        ComPtr<ID3DBlob> code;
        ComPtr<ID3DBlob> errors;
        const auto path = std::filesystem::path(SHADER_ROOT) / "hlsl" / file;
        const auto result = D3DCompileFromFile(path.c_str(),
            nullptr,
            D3D_COMPILE_STANDARD_FILE_INCLUDE,
            "main",
            profile,
            D3DCOMPILE_WARNINGS_ARE_ERRORS,
            0,
            &code,
            &errors);
        if (FAILED(result) && errors != nullptr) {
            std::cerr << static_cast<const char*>(errors->GetBufferPointer());
        }
        succeeded(result);
        return code;
    }

    class ShadowRunner {
        ComPtr<ID3D11Device> device;
        ComPtr<ID3D11DeviceContext> context;
        ComPtr<ID3D11Texture2D> depth;
        ComPtr<ID3D11Texture2D> readback;
        ComPtr<ID3D11DepthStencilView> dsv;
        ComPtr<ID3D11ShaderResourceView> srv;
        ComPtr<ID3D11Buffer> bones;
        ComPtr<ID3D11Buffer> material;
        ComPtr<ID3D11VertexShader> vs;
        ComPtr<ID3D11PixelShader> ps;
        ComPtr<ID3D11Buffer> vertices;
        ComPtr<ID3D11InputLayout> layout;

        ComPtr<ID3D11Buffer> buffer(UINT bytes, UINT flags, const void* data) {
            const D3D11_BUFFER_DESC desc{bytes, D3D11_USAGE_DEFAULT, flags, 0, 0, 0};
            const D3D11_SUBRESOURCE_DATA initial{data, 0, 0};
            ComPtr<ID3D11Buffer> result;
            succeeded(
                this->device->CreateBuffer(&desc, data == nullptr ? nullptr : &initial, &result));
            return result;
        }

        void make_depth() {
            auto desc = renderer::directx::D3D11Converter::to_texture2d_desc(
                types::ImageDescription::make_default(glm::ivec2(SIZE),
                    types::ImageFormat::D32_FLOAT,
                    types::ImageUsage::DepthStencil | types::ImageUsage::ShaderResource));
            check(desc.Format == DXGI_FORMAT_R32_TYPELESS &&
                      desc.BindFlags == (D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE),
                "sampled depth must use typeless storage with both bindings");
            succeeded(this->device->CreateTexture2D(&desc, nullptr, &this->depth));
            D3D11_DEPTH_STENCIL_VIEW_DESC dd{};
            dd.Format = DXGI_FORMAT_D32_FLOAT;
            dd.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
            succeeded(this->device->CreateDepthStencilView(this->depth.Get(), &dd, &this->dsv));
            D3D11_SHADER_RESOURCE_VIEW_DESC sd{};
            sd.Format = DXGI_FORMAT_R32_FLOAT;
            sd.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
            sd.Texture2D.MipLevels = 1;
            succeeded(this->device->CreateShaderResourceView(this->depth.Get(), &sd, &this->srv));
            desc.Usage = D3D11_USAGE_STAGING;
            desc.BindFlags = 0;
            desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
            succeeded(this->device->CreateTexture2D(&desc, nullptr, &this->readback));
        }

        void make_geometry() {
            const auto code = compile("vs_shadow_map.hlsl", "vs_5_0");
            const auto pixel = compile("ps_shadow_map.hlsl", "ps_5_0");
            succeeded(this->device->CreateVertexShader(
                code->GetBufferPointer(), code->GetBufferSize(), nullptr, &this->vs));
            succeeded(this->device->CreatePixelShader(
                pixel->GetBufferPointer(), pixel->GetBufferSize(), nullptr, &this->ps));
            const D3D11_INPUT_ELEMENT_DESC elements[] = {
                {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
                {"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},
                {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0},
                {"BONEINDICES",
                    0,
                    DXGI_FORMAT_R32G32B32A32_UINT,
                    0,
                    32,
                    D3D11_INPUT_PER_VERTEX_DATA,
                    0},
                {"BONEWEIGHTS",
                    0,
                    DXGI_FORMAT_R32G32B32A32_FLOAT,
                    0,
                    48,
                    D3D11_INPUT_PER_VERTEX_DATA,
                    0},
                {"EDGE_FLAG", 0, DXGI_FORMAT_R32_FLOAT, 0, 64, D3D11_INPUT_PER_VERTEX_DATA, 0},
                {"SKINNING_METHOD", 0, DXGI_FORMAT_R32_UINT, 0, 68, D3D11_INPUT_PER_VERTEX_DATA, 0},
                {"BLEND_CENTER",
                    0,
                    DXGI_FORMAT_R32G32B32_FLOAT,
                    0,
                    72,
                    D3D11_INPUT_PER_VERTEX_DATA,
                    0},
                {"BLEND_ANCHOR_A",
                    0,
                    DXGI_FORMAT_R32G32B32_FLOAT,
                    0,
                    84,
                    D3D11_INPUT_PER_VERTEX_DATA,
                    0},
                {"BLEND_ANCHOR_B",
                    0,
                    DXGI_FORMAT_R32G32B32_FLOAT,
                    0,
                    96,
                    D3D11_INPUT_PER_VERTEX_DATA,
                    0}};
            succeeded(this->device->CreateInputLayout(elements,
                static_cast<UINT>(std::size(elements)),
                code->GetBufferPointer(),
                code->GetBufferSize(),
                &this->layout));
            std::array<renderer::SkinnedVertex, 3> data{};
            data[0].position = {-0.8f, -0.8f, 0.25f};
            data[1].position = {0, 0.8f, 0.25f};
            data[2].position = {0.8f, -0.8f, 0.25f};
            for (auto& vertex : data) {
                vertex.normal = {0, 0, -1};
                vertex.weights = {1, 0, 0, 0};
            }
            this->vertices = this->buffer(sizeof(data), D3D11_BIND_VERTEX_BUFFER, data.data());
        }

        void make_bindings() {
            const types::UniformCamera camera{
                glm::mat4(1), glm::mat4(1), glm::mat4(1), glm::mat4(1)};
            const auto camera_buffer =
                this->buffer(sizeof(camera), D3D11_BIND_CONSTANT_BUFFER, &camera);
            const types::UniformShadow shadow{};
            const auto shadow_buffer =
                this->buffer(sizeof(shadow), D3D11_BIND_CONSTANT_BUFFER, &shadow);
            this->bones =
                this->buffer(sizeof(glm::mat4) * BONE_COUNT, D3D11_BIND_CONSTANT_BUFFER, nullptr);
            this->material =
                this->buffer(sizeof(types::UniformMaterial), D3D11_BIND_CONSTANT_BUFFER, nullptr);
            this->context->VSSetConstantBuffers(0, 1, camera_buffer.GetAddressOf());
            this->context->VSSetConstantBuffers(1, 1, this->bones.GetAddressOf());
            this->context->VSSetConstantBuffers(2, 1, shadow_buffer.GetAddressOf());
            this->context->PSSetConstantBuffers(0, 1, this->material.GetAddressOf());
            // A white texel; per-material alpha below tests the production alpha discard.
            const std::uint32_t white = 0xffffffff;
            D3D11_TEXTURE2D_DESC desc{};
            desc.Width = 1;
            desc.Height = 1;
            desc.MipLevels = 1;
            desc.ArraySize = 1;
            desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
            desc.SampleDesc.Count = 1;
            desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
            const D3D11_SUBRESOURCE_DATA data{&white, sizeof(white), sizeof(white)};
            ComPtr<ID3D11Texture2D> texture;
            ComPtr<ID3D11ShaderResourceView> view;
            succeeded(this->device->CreateTexture2D(&desc, &data, &texture));
            succeeded(this->device->CreateShaderResourceView(texture.Get(), nullptr, &view));
            this->context->PSSetShaderResources(0, 1, view.GetAddressOf());
            D3D11_SAMPLER_DESC sampler{};
            sampler.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
            sampler.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
            sampler.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
            sampler.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
            ComPtr<ID3D11SamplerState> state;
            succeeded(this->device->CreateSamplerState(&sampler, &state));
            this->context->PSSetSamplers(0, 1, state.GetAddressOf());
        }

      public:
        ShadowRunner() {
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
            this->make_depth();
            this->make_geometry();
            this->make_bindings();
            D3D11_RASTERIZER_DESC raster{};
            raster.FillMode = D3D11_FILL_SOLID;
            raster.CullMode = D3D11_CULL_NONE;
            raster.DepthClipEnable = TRUE;
            ComPtr<ID3D11RasterizerState> state;
            succeeded(this->device->CreateRasterizerState(&raster, &state));
            this->context->RSSetState(state.Get());
        }

        glm::vec4 visibility() {
            const char source[] = R"(
#include "common/shadow_visibility.hlsl"
RWStructuredBuffer<float4> results : register(u0);
[numthreads(1,1,1)] void main() {
    float3 n = float3(0,0,-1);
    results[0] = float4(
        shadow_visibility(float4(0,0,0.5,1), n,n),
        shadow_visibility(float4(0,0,0.1,1), n,n),
        shadow_visibility(float4(2,0,0.5,1), n,n),
        shadow_visibility(float4(0.9,0,0.5,1), n,n));
})";
            ComPtr<ID3DBlob> code;
            ComPtr<ID3DBlob> errors;
            const auto path = (std::filesystem::path(SHADER_ROOT) / "hlsl" / "probe.hlsl").string();
            const auto compiled = D3DCompile(source,
                sizeof(source),
                path.c_str(),
                nullptr,
                D3D_COMPILE_STANDARD_FILE_INCLUDE,
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
            ComPtr<ID3D11ComputeShader> compute;
            succeeded(this->device->CreateComputeShader(
                code->GetBufferPointer(), code->GetBufferSize(), nullptr, &compute));
            D3D11_BUFFER_DESC desc{};
            desc.ByteWidth = sizeof(glm::vec4);
            desc.BindFlags = D3D11_BIND_UNORDERED_ACCESS;
            desc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
            desc.StructureByteStride = sizeof(glm::vec4);
            ComPtr<ID3D11Buffer> output;
            ComPtr<ID3D11UnorderedAccessView> uav;
            succeeded(this->device->CreateBuffer(&desc, nullptr, &output));
            succeeded(this->device->CreateUnorderedAccessView(output.Get(), nullptr, &uav));
            desc.Usage = D3D11_USAGE_STAGING;
            desc.BindFlags = 0;
            desc.MiscFlags = 0;
            desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
            ComPtr<ID3D11Buffer> staging;
            succeeded(this->device->CreateBuffer(&desc, nullptr, &staging));
            this->context->CSSetShader(compute.Get(), nullptr, 0);
            this->context->CSSetShaderResources(3, 1, this->srv.GetAddressOf());
            this->context->CSSetUnorderedAccessViews(0, 1, uav.GetAddressOf(), nullptr);
            this->context->Dispatch(1, 1, 1);
            this->context->CopyResource(staging.Get(), output.Get());
            D3D11_MAPPED_SUBRESOURCE mapped{};
            succeeded(this->context->Map(staging.Get(), 0, D3D11_MAP_READ, 0, &mapped));
            const auto result = *static_cast<const glm::vec4*>(mapped.pData);
            this->context->Unmap(staging.Get(), 0);
            ID3D11ShaderResourceView* empty = nullptr;
            this->context->CSSetShaderResources(3, 1, &empty);
            return result;
        }

        float draw(float offset, float alpha) {
            std::array<glm::mat4, BONE_COUNT> palette;
            palette.fill(glm::translate(glm::mat4(1), glm::vec3(offset, 0, 0)));
            types::UniformMaterial mat{};
            mat.diffuse = {1, 1, 1, alpha};
            this->context->UpdateSubresource(this->bones.Get(), 0, nullptr, palette.data(), 0, 0);
            this->context->UpdateSubresource(this->material.Get(), 0, nullptr, &mat, 0, 0);
            this->context->ClearDepthStencilView(this->dsv.Get(), D3D11_CLEAR_DEPTH, 1, 0);
            this->context->OMSetRenderTargets(0, nullptr, this->dsv.Get());
            const D3D11_VIEWPORT viewport{0, 0, SIZE, SIZE, 0, 1};
            this->context->RSSetViewports(1, &viewport);
            this->context->VSSetShader(this->vs.Get(), nullptr, 0);
            this->context->PSSetShader(this->ps.Get(), nullptr, 0);
            this->context->IASetInputLayout(this->layout.Get());
            this->context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
            const UINT stride = sizeof(renderer::SkinnedVertex);
            const UINT offset_bytes = 0;
            this->context->IASetVertexBuffers(
                0, 1, this->vertices.GetAddressOf(), &stride, &offset_bytes);
            this->context->Draw(3, 0);
            this->context->OMSetRenderTargets(0, nullptr, nullptr);
            this->context->CopyResource(this->readback.Get(), this->depth.Get());
            D3D11_MAPPED_SUBRESOURCE mapped{};
            succeeded(this->context->Map(this->readback.Get(), 0, D3D11_MAP_READ, 0, &mapped));
            const auto row =
                static_cast<const std::byte*>(mapped.pData) + (SIZE / 2) * mapped.RowPitch;
            const float center = reinterpret_cast<const float*>(row)[SIZE / 2];
            this->context->Unmap(this->readback.Get(), 0);
            return center;
        }
    };

    void binding_tests() {
        for (const auto* file : {"vs_model.hlsl", "vs_shadow_map.hlsl"}) {
            const auto code = compile(file, "vs_5_0");
            ComPtr<ID3D11ShaderReflection> reflection;
            succeeded(D3DReflect(code->GetBufferPointer(),
                code->GetBufferSize(),
                IID_ID3D11ShaderReflection,
                reinterpret_cast<void**>(reflection.GetAddressOf())));
            UINT slot = 0;
            for (const auto* name : {"Camera", "Bones", "ShadowMap"}) {
                D3D11_SHADER_INPUT_BIND_DESC desc{};
                succeeded(reflection->GetResourceBindingDescByName(name, &desc));
                check(desc.BindPoint == slot, "model and shadow passes must share uniform slots");
                ++slot;
            }
        }
        for (const auto* file : {"ps_model.hlsl", "ps_shadow_map.hlsl"}) {
            const auto code = compile(file, "ps_5_0");
            ComPtr<ID3D11ShaderReflection> reflection;
            succeeded(D3DReflect(code->GetBufferPointer(),
                code->GetBufferSize(),
                IID_ID3D11ShaderReflection,
                reinterpret_cast<void**>(reflection.GetAddressOf())));
            for (const auto* name : {"Material", "model_texture", "model_sampler"}) {
                D3D11_SHADER_INPUT_BIND_DESC desc{};
                succeeded(reflection->GetResourceBindingDescByName(name, &desc));
                check(desc.BindPoint == 0, "both passes must share material/alpha texture slots");
            }
        }
    }

    void projection_tests() {
        types::ShadowBounds local;
        local.include({-2, -3, -1});
        local.include({2, 3, 1});
        const auto world = glm::translate(glm::mat4(1), glm::vec3(50, 100, -20));
        types::ShadowBounds light;
        light.include(local, types::shadow_light_view() * world);
        const auto projection = types::fit_shadow_projection(light);
        types::ShadowBounds ndc;
        ndc.include(local, projection.light_view_proj * world);
        check(ndc.minimum.x > -1 && ndc.maximum.x < 1 && ndc.minimum.y > -1 && ndc.maximum.y < 1 &&
                  ndc.minimum.z > 0 && ndc.maximum.z < 1,
            "translated model must fit D3D shadow clip volume");
    }
} // namespace

int main() {
    binding_tests();
    projection_tests();
    ShadowRunner runner;
    check(std::abs(runner.draw(0, 1) - 0.25f) < TOLERANCE,
        "opaque skinned triangle writes shadow depth");
    check(std::abs(runner.draw(0, 0) - 1.0f) < TOLERANCE,
        "transparent material does not cast shadow");
    check(std::abs(runner.draw(2, 1) - 1.0f) < TOLERANCE, "bone translation moves shadow");
    check(std::abs(runner.draw(0, 1) - 0.25f) < TOLERANCE,
        "shadow depth is refreshed on later frames");
    const auto visibility = runner.visibility();
    check(visibility.x < TOLERANCE && visibility.y > 1 - TOLERANCE &&
              visibility.z > 1 - TOLERANCE && visibility.w > 1 - TOLERANCE,
        "shadow lookup distinguishes occluded, nearer, out-of-map and unoccluded receivers");
    std::cout << "Shadow projection, skinning, depth rendering and alpha tests passed\n";
}
