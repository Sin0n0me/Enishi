#include "resource_manager.h"
#include "../d3d11_converter.h"
#include "view/render_target_view.h"
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
        , native_resource(std::make_unique<NativeGPUResource>())
        , resource_binder(std::make_unique<ResourceBinder>()) {
    }

    foundation::Option<const decltype(ResourceManager::handle_to_index)::value_type::second_type&>
    ResourceManager::get_native_resource_index(const types::RenderHandle& handle) const noexcept {
        const auto& iter = this->handle_to_index.find(handle);
        if (iter == this->handle_to_index.end()) {
            return {};
        }
        return iter->second;
    }

    INativeResourceAccessor* ResourceManager::get_native_resource_accessor(void) {
        return this->native_resource.get();
    }

    const INativeResourceAccessor* ResourceManager::get_native_resource_accessor(void) const {
        return this->native_resource.get();
    }

    IResourceBinder* ResourceManager::get_resource_binder(void) {
        return this->resource_binder.get();
    }

    const IResourceBinder* ResourceManager::get_resource_binder(void) const {
        return this->resource_binder.get();
    }

    foundation::Result<types::RenderHandle, DirectXError> ResourceManager::make_shader_reflection(
        const types::ShaderData& shader_data) {
        const auto shader_accessor = this->native_resource->get_native_shader_accessor();
        const auto [resource_index, shader_reflection] = shader_accessor->make_shader_reflection();
        auto result =
            shader_reflection->load(shader_data).add_message("シェーダーの読み込みに失敗しました");
        if (result.is_err()) {
            return result.propagation(DirectXError::ShaderReflectionError);
        }

        const auto handle = types::RenderHandle{
            .id = this->handle_allocator->create(),
            .type = types::RenderHandleType::ShaderReflection,
        };
        this->handle_to_index[handle] = ResourceIndex{
            .resource = resource_index,
        };

        return handle;
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

        const auto [resource_index, input_layout] =
            this->native_resource->get_native_input_layout_accessor()->make_native_input_layout();
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

        const auto handle = types::RenderHandle{
            .id = this->handle_allocator->create(),
            .type = types::RenderHandleType::VertexLayout,
        };
        this->handle_to_index[handle] = ResourceIndex{
            .resource = resource_index,
        };

        return handle;
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
        /*
        for (auto& argument : mesh_data.draw_args) {
            auto&& result = this->make_draw_args(std::move(argument))
                                .add_message("描画引数の作成に失敗しました");
            if (result.is_err()) {
                return std::move(result);
            }
            mesh.mesh_handles.emplace_back(result.unwrap());
        }
        */

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

        const auto buffer_accessor = this->native_resource->get_native_buffer_accessor();
        const auto [resource_index, buffer] = buffer_accessor->make_native_buffer();
        const auto device = this->context->get_device();
        const HRESULT hr = device->CreateBuffer(&desc, &init_data, buffer.GetAddressOf());
        if FAILED (hr) {
            return foundation::Error(DirectXError::BufferError, "頂点バッファの作成に失敗しました");
        }

        const auto [binding_index, binding] = this->resource_binder->make_buffer_binding();
        binding.parameter = VertexBufferParameter{
            .target = 0, // 仮
            .stride = data.stride,
            .offset = 0,
        };

        const auto handle = types::RenderHandle{
            .id = this->handle_allocator->create(),
            .type = types::RenderHandleType::Buffer,
        };
        this->handle_to_index[handle] = ResourceIndex{
            .resource = resource_index,
            .binding = binding_index,
        };

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

        const auto buffer_accessor = this->native_resource->get_native_buffer_accessor();
        const auto [resource_index, buffer] = buffer_accessor->make_native_buffer();
        const auto device = this->context->get_device();
        const HRESULT hr = device->CreateBuffer(&desc, &init_data, buffer.GetAddressOf());
        if FAILED (hr) {
            return foundation::Error(
                DirectXError::BufferError, "インデックスバッファの作成に失敗しました");
        }

        const auto [binding_index, binding] = this->resource_binder->make_buffer_binding();
        binding.parameter = IndexBufferParameter{
            .stride = data.stride,
            .offset = 0,
        };

        const auto handle = types::RenderHandle{
            .id = this->handle_allocator->create(),
            .type = types::RenderHandleType::Buffer,
        };
        this->handle_to_index[handle] = ResourceIndex{
            .resource = resource_index,
            .binding = binding_index,
        };

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

        const auto buffer_accessor = this->native_resource->get_native_buffer_accessor();
        const auto [resource_index, buffer] = buffer_accessor->make_native_buffer();
        const auto device = this->context->get_device();
        const HRESULT hr = device->CreateBuffer(&desc, &init_data, buffer.GetAddressOf());
        if FAILED (hr) {
            return foundation::Error(DirectXError::BufferError, "定数バッファの作成に失敗しました");
        }

        const auto [binding_index, binding] = this->resource_binder->make_buffer_binding();
        binding.parameter = UniformBufferParameter{
            .target = target_slot,
            .target_shader = target_shader,
        };

        const auto handle = types::RenderHandle{
            .id = this->handle_allocator->create(),
            .type = types::RenderHandleType::Buffer,
        };
        this->handle_to_index[handle] = ResourceIndex{
            .resource = resource_index,
            .binding = binding_index,
        };

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
        const auto buffer_accessor = this->native_resource->get_native_texture_accessor();
        const auto [resource_index, texture] = buffer_accessor->make_native_texture_2d();
        const auto device = this->context->get_device();
        const HRESULT hr = device->CreateTexture2D(&desc, &subresource, texture.GetAddressOf());
        if (FAILED(hr)) {
            return foundation::Error(DirectXError::BufferError, "テクスチャの作成に失敗しました");
        }

        const auto [binding_index, binding] = this->resource_binder->make_texture_binding();
        binding.target = 0;

        const auto handle = types::RenderHandle{
            .id = this->handle_allocator->create(),
            .type = types::RenderHandleType::Texture,
        };
        this->handle_to_index[handle] = ResourceIndex{
            .resource = resource_index,
            .binding = binding_index,
        };

        return handle;
    }

    foundation::Result<types::RenderHandle, DirectXError> ResourceManager::make_image(
        const types::ImageDescription& description) {
        // 先に作成
        const auto buffer_accessor = this->native_resource->get_native_texture_accessor();
        const auto [resource_index, texture] = buffer_accessor->make_native_texture_2d();
        const auto desc = D3D11Converter::to_texture2d_desc(description);

        if (description.contains(types::ImageUsage::BackBuffer)) {
            const auto swap_chain = this->context->get_swap_chain();
            const HRESULT hr = swap_chain->GetBuffer(0, IID_PPV_ARGS(texture.GetAddressOf()));
            if (FAILED(hr)) {
                return foundation::Error(
                    DirectXError::BufferError, "バックバッファの取得に失敗しました");
            }
        } else {
            const auto device = this->context->get_device();
            const HRESULT hr = device->CreateTexture2D(&desc, nullptr, texture.GetAddressOf());
            if (FAILED(hr)) {
                return foundation::Error(DirectXError::BufferError, "イメージの作成に失敗しました");
            }
        }

        const auto [binding_index, binding] = this->resource_binder->make_texture_binding();

        const auto handle = types::RenderHandle{
            .id = this->handle_allocator->create(),
            .type = types::RenderHandleType::Texture,
        };
        this->handle_to_index[handle] = ResourceIndex{
            .resource = resource_index,
            .binding = binding_index,
        };

        return handle;
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

        const auto [resource_index, rasterizer] =
            this->native_resource->get_native_rasterizer_accessor()->make_native_rasterizer();
        const auto device = this->context->get_device();
        const HRESULT hr = device->CreateRasterizerState(&desc, rasterizer.GetAddressOf());
        if (FAILED(hr)) {
            return foundation::Error(
                DirectXError::RasterizerError, "ラスタライザの作成に失敗しました");
        }

        const auto handle = types::RenderHandle{
            .id = this->handle_allocator->create(),
            .type = types::RenderHandleType::Rasterizer,
        };
        this->handle_to_index[handle] = ResourceIndex{
            .resource = resource_index,
        };

        return handle;
    }

    foundation::Result<types::RenderHandle, DirectXError> ResourceManager::make_view(
        const types::RenderHandle& image_handle, const types::ImageViewDescription& description) {
        // テクスチャから
        auto opt_index = this->get_native_resource_index(image_handle);
        if (opt_index.is_none()) {
            return foundation::Error(DirectXError::ViewError, "イメージが見つかりませんでした");
        }
        const auto& index = opt_index.unwrap();

        switch (description.type) {
            case types::ImageViewType::DepthStencil:
                return this->make_depth_stencil(index, description);
            case types::ImageViewType::RenderTarget:
                return this->make_render_target(index, description);
            case types::ImageViewType::ShaderResource:
                return this->make_shader_resource(index, description);
            case types::ImageViewType::UnorderedAccess:
                return this->make_unodered_access(index, description);
            default:
                break;
        }

        return foundation::Error(DirectXError::ViewError, "対応していないフォーマットです");
    }

    foundation::Result<types::RenderHandle, DirectXError> ResourceManager::make_viewport(
        const types::ViewportRect& config) {
        const auto viewport_accessor = this->native_resource->get_native_viewport_accessor();
        const auto [resource_index, viewport] = viewport_accessor->make_native_viewport();

        viewport.TopLeftX = static_cast<FLOAT>(config.left_top_x);
        viewport.TopLeftY = static_cast<FLOAT>(config.left_top_y);
        viewport.Width = static_cast<FLOAT>(config.width);
        viewport.Height = static_cast<FLOAT>(config.height);
        viewport.MinDepth = static_cast<FLOAT>(config.min_depth);
        viewport.MaxDepth = static_cast<FLOAT>(config.max_depth);

        const auto handle = types::RenderHandle{
            .id = this->handle_allocator->create(),
            .type = types::RenderHandleType::ViewPort,
        };
        this->handle_to_index[handle] = ResourceIndex{
            .resource = resource_index,
        };

        return handle;
    }

    foundation::Option<const types::MeshHandles&> ResourceManager::get_mesh(
        const types::HandleId handle) const {
        const auto& iter = this->meshes.find(handle);
        if (iter == this->meshes.end()) {
            return {};
        }
        return iter->second;
    }

    foundation::Result<types::RenderHandle, DirectXError> ResourceManager::make_render_target(
        const ResourceIndex image_index, const types::ImageViewDescription& description) {
        const auto texture_accessor = this->native_resource->get_native_texture_accessor();
        const auto opt_texture = texture_accessor->get_native_texture_2d(image_index.resource);
        if (opt_texture.is_none()) {
            return foundation::Error(DirectXError::ViewError, "イメージが見つかりませんでした");
        }
        const auto& texture = opt_texture.unwrap();

        // RenderTargetViewの作成
        const auto [resource_index, rtv] =
            this->native_resource->get_native_view_accessor()->make_native_render_target_view();
        const auto device = this->context->get_device();
        const HRESULT hr =
            device->CreateRenderTargetView(texture.Get(), nullptr, rtv.GetAddressOf());
        if (FAILED(hr)) {
            return foundation::Error(
                DirectXError::ViewError, "レンダーターゲットの作成に失敗しました");
        }

        const auto handle = types::RenderHandle{
            .id = this->handle_allocator->create(),
            .type = types::RenderHandleType::View,
        };

        // 外部変更用のビューの作成
        const auto configurable_index =
            this->native_resource->get_view_accessor()->make_render_target_view(
                std::make_shared<RenderTargetView>(handle, description));

        this->handle_to_index[handle] = ResourceIndex{
            .resource = resource_index,
            .configurable = configurable_index,
        };

        return handle;
    }

    foundation::Result<types::RenderHandle, DirectXError> ResourceManager::make_depth_stencil(
        const ResourceIndex image_index, const types::ImageViewDescription& description) {
        const auto texture_accessor = this->native_resource->get_native_texture_accessor();
        const auto opt_texture = texture_accessor->get_native_texture_2d(image_index.resource);
        if (opt_texture.is_none()) {
            return foundation::Error(DirectXError::ViewError, "イメージが見つかりませんでした");
        }
        const auto& texture = opt_texture.unwrap();

        // DepthStencilViewの作成
        const auto [resource_index, dsv] =
            this->native_resource->get_native_view_accessor()->make_native_depth_stencil_view();
        const auto device = this->context->get_device();
        const HRESULT hr =
            device->CreateDepthStencilView(texture.Get(), nullptr, dsv.GetAddressOf());
        if (FAILED(hr)) {
            return foundation::Error(DirectXError::ViewError, "深度ステンシルの作成に失敗しました");
        }

        const auto handle = types::RenderHandle{
            .id = this->handle_allocator->create(),
            .type = types::RenderHandleType::View,
        };

        // 外部変更用のビューの作成
        const auto configurable_index =
            this->native_resource->get_view_accessor()->make_depth_stencil_view(
                std::make_shared<DepthStencilView>(handle, description));

        this->handle_to_index[handle] = ResourceIndex{
            .resource = resource_index,
            .configurable = configurable_index,
        };

        return handle;
    }

    foundation::Result<types::RenderHandle, DirectXError> ResourceManager::make_shader_resource(
        const ResourceIndex image_index, const types::ImageViewDescription& description) {
        const auto texture_accessor = this->native_resource->get_native_texture_accessor();
        const auto opt_texture = texture_accessor->get_native_texture_2d(image_index.resource);
        if (opt_texture.is_none()) {
            return foundation::Error(DirectXError::ViewError, "イメージが見つかりませんでした");
        }
        const auto& texture = opt_texture.unwrap();

        // ShaderResourceViewの作成
        const auto [resource_index, srv] =
            this->native_resource->get_native_view_accessor()->make_native_shader_resource_view();
        const auto device = this->context->get_device();
        const HRESULT hr =
            device->CreateShaderResourceView(texture.Get(), nullptr, srv.GetAddressOf());
        if (FAILED(hr)) {
            return foundation::Error(
                DirectXError::ViewError, "シェーダーリソースの作成に失敗しました");
        }

        const auto handle = types::RenderHandle{
            .id = this->handle_allocator->create(),
            .type = types::RenderHandleType::View,
        };

        // 外部変更用のビューの作成
        const auto configurable_index =
            this->native_resource->get_view_accessor()->make_render_target_view(
                std::make_shared<RenderTargetView>(handle, description));

        this->handle_to_index[handle] = ResourceIndex{
            .resource = resource_index,
            .configurable = configurable_index,
        };

        return handle;
    }

    foundation::Result<types::RenderHandle, DirectXError> ResourceManager::make_unodered_access(
        const ResourceIndex image_index, const types::ImageViewDescription& description) {
        const auto texture_accessor = this->native_resource->get_native_texture_accessor();
        const auto opt_texture = texture_accessor->get_native_texture_2d(image_index.resource);
        if (opt_texture.is_none()) {
            return foundation::Error(DirectXError::ViewError, "イメージが見つかりませんでした");
        }
        const auto& texture = opt_texture.unwrap();

        const auto handle = types::RenderHandle{
            .id = this->handle_allocator->create(),
            .type = types::RenderHandleType::View,
        };

        // 外部変更用のビューの作成
        const auto configurable_index =
            this->native_resource->get_view_accessor()->make_render_target_view(
                std::make_shared<RenderTargetView>(handle, description));

        this->handle_to_index[handle] = ResourceIndex{
            .resource = resource_index,
            .configurable = configurable_index,
        };

        return handle;
    }

    foundation::Result<types::RenderHandle, DirectXError> ResourceManager::make_shader_from_dxbc(
        const types::ShaderKind kind, const types::ShaderData& shader_data) {
        const auto shader_accessor = this->native_resource->get_native_shader_accessor();
        const auto device = this->context->get_device();

        auto resource_index = 0u;
        switch (kind) {
            case types::ShaderKind::Vertex: {
                const auto [index, shader] = shader_accessor->make_native_vertex_shader();
                const HRESULT hr = device->CreateVertexShader(shader_data.code.data(),
                    shader_data.code.size(),
                    nullptr,
                    shader.GetAddressOf());
                if (FAILED(hr)) {
                    return foundation::Error(DirectXError::ShaderError);
                }
                resource_index = index;
            } break;
            case types::ShaderKind::Pixel: {
                const auto [index, shader] = shader_accessor->make_native_pixel_shader();
                const HRESULT hr = device->CreatePixelShader(shader_data.code.data(),
                    shader_data.code.size(),
                    nullptr,
                    shader.GetAddressOf());
                if (FAILED(hr)) {
                    return foundation::Error(DirectXError::ShaderError);
                }
                resource_index = index;
            } break;
            default:
                return foundation::Error(DirectXError::ShaderError);
        }

        const auto handle = types::RenderHandle{
            .id = this->handle_allocator->create(),
            .type = types::RenderHandleType::Shader,
        };
        this->handle_to_index[handle] = ResourceIndex{
            .resource = resource_index,
        };

        return handle;
    }
} // namespace enishi::renderer::directx