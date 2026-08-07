#include "resource_manager.h"
#include "../d3d11_converter.h"
#include "../view/render_target_view.h"
#include <foundation/log/logger.h>

namespace enishi::renderer::directx {
    ResourceManager::ResourceManager(std::shared_ptr<ID3D11Context> context)
        : context(context)
        , resource(std::make_unique<GPUResource>())
        , resource_editor(std::make_unique<ResourceEditor>()) {
    }

    GPUResourceAccessor* const ResourceManager::get_accessor(void) const {
        return this->resource_editor.get();
    }

    foundation::Result<types::RenderHandle, DirectXError> ResourceManager::make_shader_reflection(
        std::shared_ptr<types::ShaderData> shader_data) {
        const auto handle_id = this->handle_allocator.create();
        auto result = this->resource_editor->make_shader_reflection(handle_id, shader_data)
                          .add_message("shader reflectionの作成に失敗しました");
        if (result.is_err()) {
            this->handle_allocator.destroy(handle_id);
            return std::move(result).unwrap_err();
        }

        return types::RenderHandle{
            .id = handle_id,
            .type = types::RenderHandleType::ShaderReflection,
        };
    }

    foundation::Result<types::RenderHandle, DirectXError>
    ResourceManager::make_input_layout_from_shader_reflection(
        const types::RenderHandle& shader_reflection_handle) {
        if (shader_reflection_handle.type != types::RenderHandleType::ShaderReflection) {
            return foundation::Error(DirectXError::ShaderReflectionError, "不正なハンドルです");
        }

        // shader reflectionでレイアウトを作成
        auto opt_refection =
            this->resource_editor->get_shader_reflection(shader_reflection_handle.id);
        if (opt_refection.is_none()) {
            return foundation::Error(
                DirectXError::ShaderReflectionError, "シェーダーリフレクションが存在しません");
        }
        const auto& reflection = opt_refection.unwrap();
        auto shader_data = reflection->get_shader_data();
        auto input_elements = reflection->get_input_element_descs();

        Microsoft::WRL::ComPtr<ID3D11InputLayout> input_layout;
        const auto device = this->context->get_device();
        const HRESULT hr = device->CreateInputLayout(input_elements.data(),
            static_cast<uint32_t>(input_elements.size()),
            shader_data->code.data(),
            shader_data->code.size(),
            input_layout.GetAddressOf());
        if (FAILED(hr)) {
            return foundation::Error(
                DirectXError::InputLayoutError, "InputLayoutの作成に失敗しました");
        }

        const types::HandleId handle = this->handle_allocator.create();
        this->resource->input_layouts.emplace(handle, input_layout);

        return types::RenderHandle{
            .id = handle,
            .type = types::RenderHandleType::InputLayout,
        };
    }

    foundation::Result<types::RenderHandle, DirectXError> ResourceManager::make_mesh(
        types::MeshData&& mesh_data) {
        Mesh mesh{};

        // 頂点バッファ作成
        {
            auto&& result = this->make_vertex_buffer(mesh_data.vertices.get_render_data())
                                .add_message("頂点バッファの作成に失敗しました");
            if (result.is_err()) {
                return std::move(result);
            }
            mesh.mesh_handles.emplace_back(result.unwrap());
        }

        // インデックスバッファ作成
        {
            auto&& result = this->make_index_buffer(mesh_data.indices.get_render_data())
                                .add_message("インデックスバッファの作成に失敗しました");
            if (result.is_err()) {
                return std::move(result);
            }
            mesh.mesh_handles.emplace_back(result.unwrap());
        }

        // 定数バッファ作成
        for (const auto& [name, uniform] : mesh_data.uniforms) {
            auto&& result =
                this->make_uniform_buffer(uniform.get_render_data(), types::ShaderKind::Vertex, 0)
                    .add_message("定数バッファの作成に失敗しました");
            if (result.is_err()) {
                return std::move(result);
            }

            mesh.mesh_handles.emplace_back(result.unwrap());
        }

        // マテリアル
        for (auto& argument : mesh_data.draw_args) {
            auto&& result = this->make_draw_args(std::move(argument))
                                .add_message("描画引数の作成に失敗しました");
            if (result.is_err()) {
                return std::move(result);
            }
            mesh.mesh_handles.emplace_back(result.unwrap());
        }

        const types::HandleId handle = this->handle_allocator.create();

        this->meshes.emplace(handle, mesh);

        return types::RenderHandle{
            .id = handle,
            .type = types::RenderHandleType::Mesh,
        };
    }

    foundation::Result<types::RenderHandle, DirectXError> ResourceManager::make_shader(
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

    foundation::Result<types::RenderHandle, DirectXError> ResourceManager::make_texture(
        const types::TextureData& texture_data) {
        texture_data.format;

        return types::RenderHandle{};
    }

    foundation::Result<types::RenderHandle, DirectXError> ResourceManager::make_vertex_buffer(
        const types::RenderData& data) {
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
        this->resource->buffers.emplace(handle, buffer);

        return types::RenderHandle{
            .id = handle,
            .type = types::RenderHandleType::Buffer,
        };
    }

    foundation::Result<types::RenderHandle, DirectXError> ResourceManager::make_index_buffer(
        const types::RenderData& data) {
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

        const auto handle_id = this->handle_allocator.create();
        this->resource->buffers.emplace(handle_id, buffer);

        return types::RenderHandle{
            .id = handle_id,
            .type = types::RenderHandleType::Buffer,
        };
    }

    foundation::Result<types::RenderHandle, DirectXError> ResourceManager::make_uniform_buffer(
        const types::RenderData& data,
        const types::ShaderKind target_shader,
        const std::uint32_t target_slot) {
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

        Buffer buffer{UniformParameter{
            .target_shader = target_shader,
            .target_slot = target_slot,
        }};
        const auto device = this->context->get_device();
        const HRESULT hr = device->CreateBuffer(&desc, &init_data, buffer.buffer.GetAddressOf());
        if FAILED (hr) {
            return foundation::Error(DirectXError::BufferError, "定数バッファの作成に失敗しました");
        }

        const auto handle_id = this->handle_allocator.create();
        this->resource->buffers.emplace(handle_id, buffer);

        return types::RenderHandle{
            .id = handle_id,
            .type = types::RenderHandleType::Buffer,
        };
    }

    foundation::Result<types::RenderHandle, DirectXError>
    ResourceManager::make_texture_from_render_data(
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

        const auto handle_id = this->handle_allocator.create();
        this->resource->textures.emplace(handle_id, texture);

        return types::RenderHandle{
            .id = handle_id,
            .type = types::RenderHandleType::Texture,
        };
    }

    foundation::Result<types::RenderHandle, DirectXError> ResourceManager::make_image(
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
        this->resource->textures.emplace(handle, texture);

        return types::RenderHandle{
            .id = handle,
            .type = types::RenderHandleType::Texture,
        };
    }

    foundation::Result<types::RenderHandle, DirectXError> ResourceManager::make_blend_state() {
        return types::RenderHandle{};
    }

    foundation::Result<types::RenderHandle, DirectXError> ResourceManager::make_sampler() {
        return types::RenderHandle{};
    }

    foundation::Result<types::RenderHandle, DirectXError> ResourceManager::make_rasterizer(
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
        this->resource->rasterizers.emplace(handle, rasterizer);

        return types::RenderHandle{
            .id = handle,
            .type = types::RenderHandleType::Rasterizer,
        };
    }

    foundation::Result<types::RenderHandle, DirectXError> ResourceManager::make_render_target_view(
        const types::RenderHandle& image_handle, const types::ImageViewDescription& description) {
        // テクスチャから
        if (image_handle.type != types::RenderHandleType::Texture) {
            return foundation::Error(DirectXError::TargetError, "不正なテクスチャハンドルです");
        }

        const auto iter = this->resource->textures.find(image_handle.id);
        if (iter == this->resource->textures.end()) {
            return foundation::Error(DirectXError::TargetError, "イメージが見つかりませんでした");
        }
        const auto& texture = iter->second.texture;

        // 先にリソースの作成
        const types::HandleId handle_id = this->handle_allocator.create();
        auto result = this->resource->views.create(handle_id, types::ImageViewType::RenderTarget);
        if (result.is_err()) {
            return result.propagation(DirectXError::TargetError);
        }
        const auto handle = types::RenderHandle{
            .id = handle_id,
            .type = types::RenderHandleType::View,
        };

        // RenderTargetViewの作成
        auto opt_rtv = this->resource->views.get_render_target_view(handle_id);
        if (opt_rtv.is_none()) {
            return foundation::Error(DirectXError::TargetError);
        }
        auto& rtv = opt_rtv.unwrap_mut();
        const auto device = this->context->get_device();
        const HRESULT hr =
            device->CreateRenderTargetView(texture.Get(), nullptr, rtv.GetAddressOf());
        if (FAILED(hr)) {
            return foundation::Error(
                DirectXError::TargetError, "レンダーターゲットの作成に失敗しました");
        }

        // 外部変更用のレンダーターゲットの作成
        this->resource_editor->add_render_target(
            std::make_shared<RenderTargetView>(handle, description));

        return handle;
    }

    foundation::Result<types::RenderHandle, DirectXError> ResourceManager::make_viewport(
        const types::ViewportRect& config) {
        const auto id = this->resource->viewports.size();
        this->resource->viewports.emplace_back(D3D11_VIEWPORT{
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

    foundation::Result<types::RenderHandle, DirectXError> ResourceManager::make_draw_args(
        types::DrawArgs&& args) {
        const auto handle_id = this->handle_allocator.create();
        this->resource_editor->make_draw_args(handle_id, std::move(args));

        return types::RenderHandle{
            .id = handle_id,
            .type = types::RenderHandleType::Draw,
        };
    }

    foundation::Option<const Buffer&> ResourceManager::get_buffer(
        const types::HandleId handle) const {
        const auto& iter = this->resource->buffers.find(handle);
        if (iter == this->resource->buffers.end()) {
            return {};
        }
        return iter->second;
    }

    foundation::Option<const Microsoft::WRL::ComPtr<ID3D11RasterizerState>&>
    ResourceManager::get_rasterizer(const types::HandleId handle) const {
        const auto& iter = this->resource->rasterizers.find(handle);
        if (iter == this->resource->rasterizers.end()) {
            return {};
        }
        return iter->second;
    }

    foundation::Option<const Microsoft::WRL::ComPtr<ID3D11InputLayout>&>
    ResourceManager::get_input_layout(const types::HandleId handle) const {
        const auto& iter = this->resource->input_layouts.find(handle);
        if (iter == this->resource->input_layouts.end()) {
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
        return this->resource->shaders;
    }

    const ViewPool& ResourceManager::get_view_pool(void) const {
        return this->resource->views;
    }

    const std::vector<D3D11_VIEWPORT>& ResourceManager::get_viewports(void) const {
        return this->resource->viewports;
    }

    foundation::Result<types::RenderHandle, DirectXError> ResourceManager::make_shader_from_dxbc(
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
        auto& shader_pool = this->resource->shaders;

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
        auto& shader_pool = this->resource->shaders;

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