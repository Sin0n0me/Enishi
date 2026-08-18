#include "resource_manager.h"
#include "../d3d11_converter.h"
#include "../view/render_target_view.h"
#include <foundation/log/logger.h>

namespace enishi::renderer::directx {
    template <typename T>
    foundation::Option<T&> get_value(
        std::vector<T>& vec, foundation::Option<std::size_t> opt_index) {
        if (opt_index.is_none()) {
            return {};
        }
        const auto& index = opt_index.unwrap();
        if (vec.size() < index + 1) {
            return {};
        }
        return vec.at(index);
    }
    template <typename T>
    foundation::Option<const T&> get_value(
        const std::vector<T>& vec, foundation::Option<std::size_t> opt_index) {
        if (opt_index.is_none()) {
            return {};
        }
        const auto& index = opt_index.unwrap();
        if (vec.size() < index + 1) {
            return {};
        }
        return vec.at(index);
    }

    ResourceManager::ResourceManager(std::shared_ptr<ID3D11Context> context)
        : context(context)
        , handle_allocator(std::make_unique<types::HandleAllocator>())
        , native_resource(std::make_unique<NativeGPUResource>()) {
    }

    GPUResourceAccessor<DirectXError>* ResourceManager::get_accessor(void) {
        return this;
    }

    const GPUResourceAccessor<DirectXError>* ResourceManager::get_accessor(void) const {
        return this;
    }

    INativeResourceAccessor* ResourceManager::get_native_resource_accessor(void) {
        return this->native_resource.get();
    }

    const INativeResourceAccessor* ResourceManager::get_native_resource_accessor(void) const {
        return this->native_resource.get();
    }

    foundation::Result<types::RenderHandle, DirectXError> ResourceManager::make_shader_reflection(
        const types::ShaderData& shader_data) {
        const auto iter = this->hash_to_shader_refection.find(shader_data.hash());
        if (iter != this->hash_to_shader_refection.end()) {
            // return iter->second->;
        }

        auto reflection = std::make_shared<ShaderReflection>();
        if (!bool(reflection)) {
            return foundation::Error(DirectXError::ShaderReflectionError);
        }

        auto result =
            reflection->load(shader_data).add_message("シェーダーの読み込みに失敗しました");
        if (result.is_err()) {
            return result.propagation(DirectXError::ShaderReflectionError);
        }

        const auto handle_id = this->handle_allocator->create();
        this->handle_to_index[ResourceIndex{
            .type = ResourceType::ShaderReflection,
            .handle_id = handle_id,
        }] = this->shader_refections.size();

        this->shader_refections.emplace_back(reflection);

        return types::RenderHandle{
            .id = handle_id,
            .type = types::RenderHandleType::ShaderReflection,
        };
    }

    foundation::Result<types::RenderHandle, DirectXError>
    ResourceManager::make_input_layout_from_shader_data(const types::ShaderData& shader_data) {
        const auto shader_reflection = this->make_shader_reflection(shader_data);
        if (shader_reflection.is_err()) {
            return shader_reflection.propagation(DirectXError::ShaderReflectionError);
        }

        // shader reflectionでレイアウトを作成
        auto opt_refection = this->get_shader_reflection(shader_reflection.unwrap().id);
        if (opt_refection.is_none()) {
            return foundation::Error(
                DirectXError::ShaderReflectionError, "シェーダーリフレクションが存在しません");
        }

        const auto& reflection = opt_refection.unwrap();
        const auto input_layouts = reflection->get_shader_input_reflection()
                                       ->get_input_layouts(); // 保持しなければ名前が消える
        const auto input_elements = input_layouts |
                                    std::views::transform([](const ShaderInputLayout& info) {
                                        return D3D11Converter::to_input_element_description(info);
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

        const types::HandleId handle = this->handle_allocator->create();
        this->native_resource->input_layouts.emplace(handle, input_layout);

        return types::RenderHandle{
            .id = handle,
            .type = types::RenderHandleType::VertexLayout,
        };
    }

    foundation::Result<types::RenderHandle, DirectXError> ResourceManager::make_mesh(
        types::MeshData&& mesh_data, const std::vector<types::RenderHandle>& shader_reflections) {
        types::MeshHandles mesh{};

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

        using ReturnType = foundation::Option<IShaderReflection<DirectXError>*>;

        // シェーダーリフレクションから定数バッファやサンプラーなどのバインド位置を取得
        auto reflections =
            shader_reflections |
            std::views::transform([this](const types::RenderHandle& handle) -> ReturnType {
                if (handle.type != types::RenderHandleType::ShaderReflection) {
                    return {};
                }
                return this->get_shader_reflection(handle.id);
            }) |
            std::views::filter([](const ReturnType& reflection) { return reflection.is_some(); }) |
            std::views::transform([this](const ReturnType& opt_reflection) {
                const auto reflection = opt_reflection.unwrap();
                return std::tuple{
                    reflection->get_shader_kind(),
                    reflection->get_shader_input_reflection()->get_input_resources(),
                };
            }) |
            std::ranges::to<std::vector>();

        // 定数バッファ作成
        const auto pre_size = mesh.mesh_handles.size();
        for (auto& [kind, resources] : reflections) {
            for (auto& resource : resources) {
                switch (resource.type) {
                    case ShaderInputResourceType::UniformBuffer: {
                        const auto& iter = mesh_data.uniforms.find(resource.name);
                        if (iter == mesh_data.uniforms.end()) {
                            continue;
                        }
                        const auto& uniform = iter->second;

                        auto&& result = this->make_uniform_buffer(
                                                uniform.get_render_data(), kind, resource.binding)
                                            .add_message("定数バッファの作成に失敗しました");
                        if (result.is_err()) {
                            return result;
                        }

                        mesh.mesh_handles.emplace_back(result.unwrap());
                    } break;
                    case ShaderInputResourceType::Texture: {
                        // this->make_texture();
                    } break;
                    case ShaderInputResourceType::Sampler: {
                        // this->make_sampler();
                    } break;
                    default:
                        foundation::Logger::warning("未対応のリソースです");
                        break;
                }
            }
        }
        const auto unifrom_size = mesh.mesh_handles.size() - pre_size;
        if (unifrom_size != mesh_data.uniforms.size()) {
            return foundation::Error(DirectXError::BufferError, "定数バッファの作成に失敗しました");
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

        const types::HandleId handle = this->handle_allocator->create();

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

        const auto buffer_accessor = this->native_resource->get_buffer_accessor();
        const auto [index, buffer] = buffer_accessor->make_buffer();
        const auto device = this->context->get_device();
        const HRESULT hr = device->CreateBuffer(&desc, &init_data, buffer.GetAddressOf());
        if FAILED (hr) {
            return foundation::Error(DirectXError::BufferError, "頂点バッファの作成に失敗しました");
        }

        const auto handle = types::RenderHandle{
            .id = this->handle_allocator->create(),
            .type = types::RenderHandleType::Buffer,
        };
        this->handle_to_index[handle] = index;

        return handle;
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

        const auto buffer_accessor = this->native_resource->get_buffer_accessor();
        const auto [index, buffer] = buffer_accessor->make_buffer();
        const auto device = this->context->get_device();
        const HRESULT hr = device->CreateBuffer(&desc, &init_data, buffer.GetAddressOf());
        if FAILED (hr) {
            return foundation::Error(
                DirectXError::BufferError, "インデックスバッファの作成に失敗しました");
        }

        const auto handle = types::RenderHandle{
            .id = this->handle_allocator->create(),
            .type = types::RenderHandleType::Buffer,
        };
        this->handle_to_index[handle] = index;

        return handle;
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
        const auto buffer_accessor = this->native_resource->get_buffer_accessor();
        const auto [index, buffer] = buffer_accessor->make_buffer();
        const auto device = this->context->get_device();
        const HRESULT hr = device->CreateBuffer(&desc, &init_data, buffer.GetAddressOf());
        if FAILED (hr) {
            return foundation::Error(DirectXError::BufferError, "定数バッファの作成に失敗しました");
        }

        const auto handle = types::RenderHandle{
            .id = this->handle_allocator->create(),
            .type = types::RenderHandleType::Buffer,
        };
        this->handle_to_index[handle] = index;

        return handle;
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
        const auto buffer_accessor = this->native_resource->get_texture_accessor();
        const auto [index, texture] = buffer_accessor->make();
        const auto device = this->context->get_device();
        const HRESULT hr = device->CreateTexture2D(&desc, &subresource, texture.GetAddressOf());
        if (FAILED(hr)) {
            return foundation::Error(DirectXError::BufferError, "テクスチャの作成に失敗しました");
        }

        const auto handle = types::RenderHandle{
            .id = this->handle_allocator->create(),
            .type = types::RenderHandleType::Texture,
        };
        this->handle_to_index[handle] = index;

        return handle;
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

        const types::HandleId handle = this->handle_allocator->create();
        this->native_resource->textures.emplace(handle, texture);

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

        const types::HandleId handle = this->handle_allocator->create();
        this->native_resource->rasterizers.emplace(handle, rasterizer);

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

        const auto iter = this->native_resource->textures.find(image_handle.id);
        if (iter == this->native_resource->textures.end()) {
            return foundation::Error(DirectXError::TargetError, "イメージが見つかりませんでした");
        }
        const auto& texture = iter->second.texture;

        // 先にリソースの作成
        const types::HandleId handle_id = this->handle_allocator->create();
        auto result =
            this->native_resource->views.create(handle_id, types::ImageViewType::RenderTarget);
        if (result.is_err()) {
            return result.propagation(DirectXError::TargetError);
        }
        const auto handle = types::RenderHandle{
            .id = handle_id,
            .type = types::RenderHandleType::View,
        };

        // RenderTargetViewの作成
        auto opt_rtv = this->native_resource->views.get_render_target_view(handle_id);
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
        this->handle_to_index[ResourceIndex{
            .type = ResourceType::RenderTarget,
            .handle_id = handle.id,
        }] = this->render_targets.size();
        this->render_targets.emplace_back(std::make_shared<RenderTargetView>(handle, description));

        return handle;
    }

    foundation::Result<types::RenderHandle, DirectXError> ResourceManager::make_viewport(
        const types::ViewportRect& config) {
        const auto id = this->native_resource->viewports.size();
        this->native_resource->viewports.emplace_back(D3D11_VIEWPORT{
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
        const auto handle_id = this->handle_allocator->create();
        this->handle_to_index[ResourceIndex{
            .type = ResourceType::DrawArgs,
            .handle_id = handle_id,
        }] = this->draw_args.size();

        this->draw_args.emplace_back(std::move(args));

        return types::RenderHandle{
            .id = handle_id,
            .type = types::RenderHandleType::Draw,
        };
    }

    foundation::Option<std::shared_ptr<platform::IRenderTargetView>>
    ResourceManager::get_render_target(const types::HandleId handle) const {
        return get_value(this->render_targets,
            this->get_index(ResourceIndex{
                .type = ResourceType::RenderTarget,
                .handle_id = handle,
            }));
    }

    const std::vector<std::shared_ptr<platform::IRenderTargetView>>&
    ResourceManager::get_render_targets(void) const {
        return this->render_targets;
    }

    foundation::Option<const types::DrawArgs&> ResourceManager::get_draw_args(
        const types::HandleId handle) const {
        return get_value(this->draw_args,
            this->get_index(ResourceIndex{
                .type = ResourceType::DrawArgs,
                .handle_id = handle,
            }));
    }

    foundation::Option<types::DrawArgs&> ResourceManager::get_draw_args(
        const types::HandleId handle) {
        return get_value(this->draw_args,
            this->get_index(ResourceIndex{
                .type = ResourceType::DrawArgs,
                .handle_id = handle,
            }));
    }

    foundation::Option<const IShaderReflection<DirectXError>*>
    ResourceManager::get_shader_reflection(const types::HandleId handle) const {
        return get_value(this->shader_refections,
            this->get_index(ResourceIndex{
                .type = ResourceType::ShaderReflection,
                .handle_id = handle,
            }))
            .transform([](decltype(ResourceManager::shader_refections)::value_type reflection) {
                return reflection.get();
            });
    }

    foundation::Option<IShaderReflection<DirectXError>*> ResourceManager::get_shader_reflection(
        const types::HandleId handle) {
        return get_value(this->shader_refections,
            this->get_index(ResourceIndex{
                .type = ResourceType::ShaderReflection,
                .handle_id = handle,
            }))
            .transform([](decltype(ResourceManager::shader_refections)::value_type reflection) {
                return reflection.get();
            });
    }

    foundation::Option<Buffer&> ResourceManager::get_buffer(const types::HandleId handle) {
        const auto& iter = this->native_resource->buffers.find(handle);
        if (iter == this->native_resource->buffers.end()) {
            return {};
        }
        return iter->second;
    }

    foundation::Option<const Buffer&> ResourceManager::get_buffer(
        const types::HandleId handle) const {
        const auto& iter = this->native_resource->buffers.find(handle);
        if (iter == this->native_resource->buffers.end()) {
            return {};
        }
        return iter->second;
    }

    foundation::Option<const Microsoft::WRL::ComPtr<ID3D11RasterizerState>&>
    ResourceManager::get_rasterizer(const types::HandleId handle) const {
        const auto& iter = this->native_resource->rasterizers.find(handle);
        if (iter == this->native_resource->rasterizers.end()) {
            return {};
        }
        return iter->second;
    }

    foundation::Option<const Microsoft::WRL::ComPtr<ID3D11InputLayout>&>
    ResourceManager::get_input_layout(const types::HandleId handle) const {
        const auto& iter = this->native_resource->input_layouts.find(handle);
        if (iter == this->native_resource->input_layouts.end()) {
            return {};
        }
        return iter->second;
    }

    foundation::Option<const types::MeshHandles&> ResourceManager::get_mesh(
        const types::HandleId handle) const {
        const auto& iter = this->meshes.find(handle);
        if (iter == this->meshes.end()) {
            return {};
        }
        return iter->second;
    }

    const ShaderPool& ResourceManager::get_shader_pool(void) const {
        return this->native_resource->shaders;
    }

    const ViewPool& ResourceManager::get_view_pool(void) const {
        return this->native_resource->views;
    }

    const std::vector<D3D11_VIEWPORT>& ResourceManager::get_viewports(void) const {
        return this->native_resource->viewports;
    }

    foundation::Option<std::size_t> ResourceManager::get_index(const ResourceIndex& index) const {
        const auto& iter = this->handle_to_index.find(index);
        if (iter == this->handle_to_index.end()) {
            return {};
        }
        return iter->second;
    }

    foundation::Result<types::RenderHandle, DirectXError> ResourceManager::make_shader_from_dxbc(
        const types::ShaderKind kind, const types::ShaderData& shader_data) {
        const auto handle = this->handle_allocator->create();

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
            this->handle_allocator->destroy(handle);
            return std::move(result).unwrap_err();
        }

        return types::RenderHandle{
            .id = handle,
            .type = types::RenderHandleType::Shader,
        };
    }

    foundation::VoidResult<DirectXError> ResourceManager::make_vertex_shader(

        const types::ShaderData& shader_data, const types::HandleId handle) {
        auto& shader_pool = this->native_resource->shaders;

        // 先に作成
        auto&& result = shader_pool.create(handle, types::ShaderKind::Vertex);
        if (result.is_err()) {
            this->handle_allocator->destroy(handle);
            return result;
        };

        auto opt_shader = shader_pool.get_vertex_shader(handle);
        const auto device = this->context->get_device();
        const HRESULT hr = device->CreateVertexShader(shader_data.code.data(),
            shader_data.code.size(),
            nullptr,
            opt_shader.unwrap_mut().GetAddressOf());
        if (FAILED(hr)) {
            this->handle_allocator->destroy(handle);
            return foundation::Error(DirectXError::ShaderError);
        }

        return {};
    }

    foundation::VoidResult<DirectXError> ResourceManager::make_pixel_shader(
        const types::ShaderData& shader_data, const types::HandleId handle) {
        auto& shader_pool = this->native_resource->shaders;

        // 先に作成
        auto&& result = shader_pool.create(handle, types::ShaderKind::Pixel);
        if (result.is_err()) {
            this->handle_allocator->destroy(handle);
            return std::move(result);
        };

        auto opt_shader = shader_pool.get_pixel_shader(handle);
        const auto device = this->context->get_device();
        const HRESULT hr = device->CreatePixelShader(shader_data.code.data(),
            shader_data.code.size(),
            nullptr,
            opt_shader.unwrap_mut().GetAddressOf());
        if (FAILED(hr)) {
            this->handle_allocator->destroy(handle);
            return foundation::Error(DirectXError::ShaderError);
        }

        return {};
    }
} // namespace enishi::renderer::directx