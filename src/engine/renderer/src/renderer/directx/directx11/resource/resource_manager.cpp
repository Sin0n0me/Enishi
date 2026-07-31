#include "resource_manager.h"
#include "../d3d11_converter.h"
#include <foundation/log/logger.h>
#include <renderer/directx/directx11/shader/shader_refrection.h>

namespace enishi::renderer::directx {
    ResourceManager::ResourceManager(std::shared_ptr<ID3D11Context> context)
        : context(context) {
    }

    ResourceManager::Result ResourceManager::make_input_layout_from_shader(
        const types::ShaderData& shader_data) {
        // shader reflectionでレイアウトを作成
        auto result = ShaderReflection::make(shader_data)
                          .add_message("shader reflectionの作成に失敗しました");
        if (result.is_err()) {
            return std::move(result).unwrap_err();
        }
        const auto& refection = result.unwrap();

        // 変換
        const auto input_elements = refection.get_input_element_descs() |
                                    std::views::transform([](const InputElementDescription& desc) {
                                        return desc.description;
                                    }) |
                                    std::ranges::to<std::vector>();

        Microsoft::WRL::ComPtr<ID3D11InputLayout> input_layout;
        const auto device = this->context->get_device();
        const HRESULT hr = device->CreateInputLayout(input_elements.data(),
            static_cast<uint32_t>(input_elements.size()),
            shader_data.code.data(),
            shader_data.code.size(),
            input_layout.GetAddressOf());
        if (FAILED(hr)) {
            return foundation::Error(
                DirectXError::InputLayoutError, "InputLayoutの作成に失敗しました");
        }

        const types::HandleId handle = this->handle_allocator.create();
        this->resource.input_layouts.emplace(handle, input_layout);

        return types::RenderHandle{
            .id = handle,
            .type = types::RenderHandleType::InputLayout,
        };
    }

    ResourceManager::Result ResourceManager::make_mesh(const types::MeshData& mesh_data) {
        // 頂点バッファ作成
        auto vertex_handle = types::RenderHandle{.id = types::INVALID_HANDLE_ID};
        auto&& vertex_result = this->make_vertex_buffer(mesh_data.vertices)
                                   .add_message("メッシュの作成に失敗しました");
        if (vertex_result.is_err()) {
            return std::move(vertex_result);
        }
        vertex_handle = vertex_result.unwrap();

        // インデックスバッファ作成
        auto index_handle = types::RenderHandle{.id = types::INVALID_HANDLE_ID};
        auto&& index_result =
            this->make_index_buffer(mesh_data.indices).add_message("メッシュの作成に失敗しました");
        if (index_result.is_err()) {
            return std::move(index_result);
        }
        index_handle = index_result.unwrap();

        const types::HandleId handle = this->handle_allocator.create();
        Mesh mesh{};
        mesh.mesh_handles.push_back(vertex_handle);
        mesh.mesh_handles.push_back(index_handle);

        this->meshes.emplace(handle, mesh);

        return types::RenderHandle{
            .id = handle,
            .type = types::RenderHandleType::Mesh,
        };
    }

    ResourceManager::Result ResourceManager::make_shader(
        const types::ShaderKind kind, const types::ShaderData& shader_data) {
        switch (shader_data.binary_type) {
            case types::ShaderBinaryType::DXBC: {
                return this->make_shader_from_dxbc(kind, shader_data);
            } break;
            case types::ShaderBinaryType::DXIL: {
            } break;
            case types::ShaderBinaryType::SPIR_V: {
            } break;
            default:
                break;
        }

        return foundation::Error(DirectXError::ShaderError);
    }

    ResourceManager::Result ResourceManager::make_texture(const types::TextureData& texture_data) {
        texture_data.format;

        return types::RenderHandle{};
    }

    ResourceManager::Result ResourceManager::make_vertex_buffer(const types::RenderData& data) {
        const D3D11_BUFFER_DESC desc{
            .ByteWidth = static_cast<UINT>(data.byte_width()),
            .Usage = D3D11_USAGE_DYNAMIC,
            .BindFlags = D3D11_BIND_VERTEX_BUFFER,
            .CPUAccessFlags = D3D11_CPU_ACCESS_WRITE,
        };
        const D3D11_SUBRESOURCE_DATA init_data{
            .pSysMem = data.raw_data(),
        };

        Buffer buffer{VertexParameter{
            .stride = data.stride,
            .offset = 0,
            .target_slot = 0,
        }};

        const auto device = this->context->get_device();
        const HRESULT hr = device->CreateBuffer(&desc, &init_data, buffer.buffer.GetAddressOf());
        if FAILED (hr) {
            return foundation::Error(DirectXError::BufferError, "頂点バッファの作成に失敗しました");
        }

        const types::HandleId handle = this->handle_allocator.create();
        this->resource.buffers.emplace(handle, buffer);

        return types::RenderHandle{
            .id = handle,
            .type = types::RenderHandleType::Buffer,
        };
    }

    ResourceManager::Result ResourceManager::make_index_buffer(const types::RenderData& data) {
        const D3D11_BUFFER_DESC desc{
            .ByteWidth = static_cast<UINT>(data.byte_width()),
            .Usage = D3D11_USAGE_DEFAULT,
            .BindFlags = D3D11_BIND_INDEX_BUFFER,
            .CPUAccessFlags = 0,
        };
        const D3D11_SUBRESOURCE_DATA init_data{
            .pSysMem = data.raw_data(),
        };

        const DXGI_FORMAT format = [](const std::uint32_t stride) -> DXGI_FORMAT {
            switch (stride) {
                case 1:
                    return DXGI_FORMAT::DXGI_FORMAT_R8_UINT;
                case 2:
                    return DXGI_FORMAT::DXGI_FORMAT_R16_UINT;
                case 4:
                    return DXGI_FORMAT::DXGI_FORMAT_R32_UINT;
                default:
                    break;
            }
            return DXGI_FORMAT::DXGI_FORMAT_UNKNOWN;
        }(data.stride);

        Buffer buffer{IndexParameter{.format = format}};
        const auto device = this->context->get_device();
        const HRESULT hr = device->CreateBuffer(&desc, &init_data, buffer.buffer.GetAddressOf());
        if FAILED (hr) {
            return foundation::Error(
                DirectXError::BufferError, "インデックスバッファの作成に失敗しました");
        }

        const types::HandleId handle = this->handle_allocator.create();
        this->resource.buffers.emplace(handle, buffer);

        return types::RenderHandle{
            .id = handle,
            .type = types::RenderHandleType::Buffer,
        };
    }

    ResourceManager::Result ResourceManager::make_constant_buffer(const types::RenderData& data) {
        const D3D11_BUFFER_DESC desc{
            .ByteWidth = static_cast<UINT>(data.byte_width()),
            .Usage = D3D11_USAGE_DEFAULT,
            .BindFlags = D3D11_BIND_CONSTANT_BUFFER,
            .CPUAccessFlags = 0,
        };
        const D3D11_SUBRESOURCE_DATA init_data{
            .pSysMem = data.raw_data(),
            .SysMemPitch = 0,
            .SysMemSlicePitch = data.stride,
        };

        // TODO
        Buffer buffer{UniformParameter{
            .target_shader = ShaderType::Vertex,
            .target_slot = 0,
        }};
        const auto device = this->context->get_device();
        const HRESULT hr = device->CreateBuffer(&desc, &init_data, buffer.buffer.GetAddressOf());
        if FAILED (hr) {
            return foundation::Error(DirectXError::BufferError, "定数バッファの作成に失敗しました");
        }

        const types::HandleId handle = this->handle_allocator.create();
        this->resource.buffers.emplace(handle, buffer);

        return types::RenderHandle{
            .id = handle,
            .type = types::RenderHandleType::Buffer,
        };
    }

    ResourceManager::Result ResourceManager::make_texture(
        const types::RenderData& data, const std::uint32_t width, const std::uint32_t height) {
        const D3D11_SUBRESOURCE_DATA subresource{
            .pSysMem = data.raw_data(),
            .SysMemPitch = width * data.stride,
        };
        constexpr DXGI_SAMPLE_DESC sample{.Count = 1};
        const D3D11_TEXTURE2D_DESC desc{
            .Width = width,
            .Height = height,
            .MipLevels = 1,
            .ArraySize = 1,
            .Format = DXGI_FORMAT_R8G8B8A8_UNORM,
            .SampleDesc = sample,
            .Usage = D3D11_USAGE_DEFAULT,
            .BindFlags = D3D11_BIND_SHADER_RESOURCE,
        };

        // 先に作成
        Texture texture{
            .texture_type = TextureType::Texture2D,
        };
        const auto device = this->context->get_device();
        const HRESULT hr =
            device->CreateTexture2D(&desc, &subresource, texture.texture.GetAddressOf());
        if (FAILED(hr)) {
            return foundation::Error(DirectXError::BufferError, "テクスチャの作成に失敗しました");
        }

        const types::HandleId handle = this->handle_allocator.create();
        this->resource.textures.emplace(handle, texture);

        return types::RenderHandle{
            .id = handle,
            .type = types::RenderHandleType::Texture,
        };
    }

    ResourceManager::Result ResourceManager::make_image(
        const types::ImageDescription& description) {
        // 先に作成
        Texture texture{
            .texture_type = TextureType::Texture2D,
        };
        const auto desc = D3D11Converter::to_texture2d_desc(description);

        if (description.contains(types::ImageUsage::BackBuffer)) {
            const auto swap_chain = this->context->get_swap_chain();
            const HRESULT hr =
                swap_chain->GetBuffer(0, IID_PPV_ARGS(texture.texture.GetAddressOf()));
            if (FAILED(hr)) {
                return foundation::Error(
                    DirectXError::BufferError, "バックバッファの取得に失敗しました");
            }
        } else {
            const auto device = this->context->get_device();
            const HRESULT hr =
                device->CreateTexture2D(&desc, nullptr, texture.texture.GetAddressOf());
            if (FAILED(hr)) {
                return foundation::Error(DirectXError::BufferError, "イメージの作成に失敗しました");
            }
        }

        const types::HandleId handle = this->handle_allocator.create();
        this->resource.textures.emplace(handle, texture);

        return types::RenderHandle{
            .id = handle,
            .type = types::RenderHandleType::Texture,
        };
    }

    ResourceManager::Result ResourceManager::make_rasterizer(
        const types::RasterizerDescription& description) {
        const auto desc = D3D11Converter::to_rasterizer_desc(description);

        Microsoft::WRL::ComPtr<ID3D11RasterizerState> rasterizer;
        const auto device = this->context->get_device();
        const HRESULT hr = device->CreateRasterizerState(&desc, rasterizer.GetAddressOf());
        if (FAILED(hr)) {
            return foundation::Error(
                DirectXError::RasterizerError, "ラスタライザの作成に失敗しました");
        }

        const types::HandleId handle = this->handle_allocator.create();
        this->resource.rasterizers.emplace(handle, rasterizer);

        return types::RenderHandle{
            .id = handle,
            .type = types::RenderHandleType::Rasterizer,
        };
    }

    ResourceManager::Result ResourceManager::make_render_target_view(
        const types::RenderHandle& image_handle, const types::ImageViewDescription& description) {
        //
        if (image_handle.type != types::RenderHandleType::Texture) {
            return foundation::Error(DirectXError::TargetError, "不正なハンドルです");
        }

        const auto iter = this->resource.textures.find(image_handle.id);
        if (iter == this->resource.textures.end()) {
            return foundation::Error(DirectXError::TargetError, "イメージが見つかりませんでした");
        }
        const auto& texture = iter->second.texture;

        // 先にリソースの作成
        const types::HandleId handle = this->handle_allocator.create();
        auto result = this->resource.views.create(handle, types::ImageViewType::RenderTarget);
        if (result.is_err()) {
            return result.propagation(DirectXError::TargetError);
        }

        // RenderTargetViewの作成
        const auto opt_rtv = this->resource.views.get_address_render_target_view(handle);
        if (opt_rtv.is_none()) {
            return result.propagation(DirectXError::TargetError);
        }
        auto rtv = opt_rtv.value();
        const auto device = this->context->get_device();
        const HRESULT hr = device->CreateRenderTargetView(texture.Get(), nullptr, rtv);
        if (FAILED(hr)) {
            return foundation::Error(
                DirectXError::TargetError, "レンダーターゲットの作成に失敗しました");
        }

        return types::RenderHandle{
            .id = handle,
            .type = types::RenderHandleType::View,
        };
    }

    ResourceManager::Result ResourceManager::make_viewport(const types::ViewportRect& config) {
        const auto id = this->resource.viewports.size();
        this->resource.viewports.emplace_back(D3D11_VIEWPORT{
            .TopLeftX = static_cast<FLOAT>(config.left_top_x),
            .TopLeftY = static_cast<FLOAT>(config.left_top_y),
            .Width = static_cast<FLOAT>(config.width),
            .Height = static_cast<FLOAT>(config.height),
            .MinDepth = static_cast<FLOAT>(config.min_depth),
            .MaxDepth = static_cast<FLOAT>(config.max_depth),
        });

        return types::RenderHandle{
            .id = static_cast<types::HandleId>(id),
            .type = types::RenderHandleType::ViewPort,
        };
    }

    foundation::Option<const Buffer&> ResourceManager::get_buffer(
        const types::HandleId handle) const {
        const auto& iter = this->resource.buffers.find(handle);
        if (iter == this->resource.buffers.end()) {
            return {};
        }
        return iter->second;
    }

    foundation::Option<const Microsoft::WRL::ComPtr<ID3D11RasterizerState>&>
    ResourceManager::get_rasterizer(const types::HandleId handle) const {
        const auto& iter = this->resource.rasterizers.find(handle);
        if (iter == this->resource.rasterizers.end()) {
            return {};
        }
        return iter->second;
    }

    foundation::Option<const Microsoft::WRL::ComPtr<ID3D11InputLayout>&>
    ResourceManager::get_input_layout(const types::HandleId handle) const {
        const auto& iter = this->resource.input_layouts.find(handle);
        if (iter == this->resource.input_layouts.end()) {
            return {};
        }
        return iter->second;
    }

    foundation::Option<const Mesh&> ResourceManager::get_mesh(const types::HandleId handle) const {
        const auto& iter = this->meshes.find(handle);
        if (iter == this->meshes.end()) {
            return {};
        }
        return iter->second;
    }

    const ShaderPool& ResourceManager::get_shader_pool(void) const {
        return this->resource.shaders;
    }

    const ViewPool& ResourceManager::get_view_pool(void) const {
        return this->resource.views;
    }

    const std::vector<D3D11_VIEWPORT>& ResourceManager::get_viewports(void) const {
        return this->resource.viewports;
    }

    ResourceManager::Result ResourceManager::make_shader_from_dxbc(
        const types::ShaderKind kind, const types::ShaderData& shader_data) {
        const auto handle = this->handle_allocator.create();

        foundation::VoidResult<DirectXError>&& result =
            foundation::Error(DirectXError::ShaderError);
        switch (kind) {
            case types::ShaderKind::Vertex: {
                result = this->make_vertex_shader(shader_data, handle);
            } break;
            case types::ShaderKind::Pixel: {
                result = this->make_pixel_shader(shader_data, handle);
            } break;
            default:
                break;
        }

        // エラーがあればハンドルは削除
        if (result.is_err()) {
            this->handle_allocator.destroy(handle);
            return std::move(result).unwrap_err();
        }

        return types::RenderHandle{
            .id = handle,
            .type = types::RenderHandleType::Shader,
        };
    }

    foundation::VoidResult<DirectXError> ResourceManager::make_vertex_shader(

        const types::ShaderData& shader_data, const types::HandleId handle) {
        auto& shader_pool = this->resource.shaders;

        // 先に作成
        auto&& result = shader_pool.create(handle, types::ShaderKind::Vertex);
        if (result.is_err()) {
            this->handle_allocator.destroy(handle);
            return result;
        };

        auto opt_shader = shader_pool.get_vertex_shader(handle);
        const auto device = this->context->get_device();
        const HRESULT hr = device->CreateVertexShader(shader_data.code.data(),
            shader_data.code.size(),
            nullptr,
            opt_shader.unwrap_mut().GetAddressOf());
        if (FAILED(hr)) {
            this->handle_allocator.destroy(handle);
            return foundation::Error(DirectXError::ShaderError);
        }

        return {};
    }

    foundation::VoidResult<DirectXError> ResourceManager::make_pixel_shader(

        const types::ShaderData& shader_data, const types::HandleId handle) {
        auto& shader_pool = this->resource.shaders;

        // 先に作成
        auto&& result = shader_pool.create(handle, types::ShaderKind::Pixel);
        if (result.is_err()) {
            this->handle_allocator.destroy(handle);
            return std::move(result);
        };

        auto opt_shader = shader_pool.get_pixel_shader(handle);
        const auto device = this->context->get_device();
        const HRESULT hr = device->CreatePixelShader(shader_data.code.data(),
            shader_data.code.size(),
            nullptr,
            opt_shader.unwrap_mut().GetAddressOf());
        if (FAILED(hr)) {
            this->handle_allocator.destroy(handle);
            return foundation::Error(DirectXError::ShaderError);
        }

        return {};
    }
} // namespace enishi::renderer::directx