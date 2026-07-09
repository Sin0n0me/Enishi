#pragma once
#include "buffer.h"
#include "shader.h"
#include "texture.h"
#include "view.h"
#include <d3d11.h>
#include <engine_types/handle/handle_type.h>
#include <unordered_map>
#include <vector>
#include <wrl/client.h>

namespace enishi::renderer::directx {
    namespace {
        template <typename T> using ResourceMap = std::unordered_map<types::HandleId, T>;
    }

    struct GPUResource {
        ResourceMap<Microsoft::WRL::ComPtr<ID3D11InputLayout>> input_layouts;
        ResourceMap<Microsoft::WRL::ComPtr<ID3D11RasterizerState>> rasterizers;
        ResourceMap<Buffer> buffers;
        ResourceMap<Texture> textures;
        ShaderPool shaders;
        ViewPool views;
        std::vector<D3D11_VIEWPORT> viewports;
    };
} // namespace enishi::renderer::directx