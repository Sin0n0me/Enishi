#include "resource_manager.h"
#include "../d3d11_converter.h"
#include "buffer/buffer_interface_maker.h"
#include "resource_manager.h"
#include "view/depth_stencil_view.h"
#include "view/render_target_view.h"
#include "view/shader_resource_view.h"
#include "view/unodered_access_view.h"
#include <foundation/log/logger.h>
#include <foundation/str/string_builder.h>
#include <ranges>

namespace enishi::renderer::directx {
    ResourceManager::ResourceManager(std::shared_ptr<ID3D11Context> context)
        : context(context)
        , handle_mapper(std::make_unique<RenderHandleMapper>())
        , native_resource(std::make_unique<NativeGPUResource>())
        , resource_binder(std::make_unique<ResourceBinder>()) {
    }

    const platform::IRenderHandleMapper* ResourceManager::get_render_handle_mapper(void) const {
        return this->handle_mapper.get();
    }

    INativeResourceAccessor* ResourceManager::get_native_resource_accessor(void) {
        return this->native_resource.get();
    }

    const INativeResourceAccessor* ResourceManager::get_native_resource_accessor(void) const {
        return this->native_resource.get();
    }

    platform::IResourceBinder* ResourceManager::get_resource_binder(void) {
        return this->resource_binder.get();
    }

    const platform::IResourceBinder* ResourceManager::get_resource_binder(void) const {
        return this->resource_binder.get();
    }

    foundation::Result<types::RenderHandle, platform::RenderError>
    ResourceManager::make_shader_reflection(const types::ShaderData& shader_data) {
        const auto [config_index, shader_reflection] =
            this->native_resource->get_shader_accessor()->make_shader_reflection(
                std::make_shared<D3D11ShaderReflection>());
        auto result =
            shader_reflection->load(shader_data).add_message("シェーダーの読み込みに失敗しました");
        if (result.is_err()) {
            return result.propagation(platform::RenderError::MakeError);
        }

        return this->handle_mapper->make(types::RenderHandleType::ShaderReflection,
            types::ResourceHandles{
                .configurable = config_index,
            });
    }

    foundation::Result<types::RenderHandle, platform::RenderError>
    ResourceManager::make_input_layout_from_shader_data(const types::ShaderData& shader_data) {
        const auto shader_reflection = this->make_shader_reflection(shader_data);
        if (shader_reflection.is_err()) {
            return shader_reflection.propagation(platform::RenderError::MakeError);
        }
        const auto opt_index = this->handle_mapper->get(shader_reflection.unwrap());
        if (opt_index.is_none()) {
            return foundation::Error(
                platform::RenderError::MakeError, "シェーダーリフレクションが存在しません");
        }
        const auto& index = opt_index.unwrap();
        auto opt_refection =
            this->native_resource->get_shader_accessor()->get_shader_reflection(index.configurable);
        if (opt_refection.is_none()) {
            return foundation::Error(
                platform::RenderError::MakeError, "シェーダーリフレクションが存在しません");
        }

        // shader reflectionでレイアウトを作成
        const auto& reflection = opt_refection.unwrap();
        const auto input_layouts = reflection->get_shader_input_reflection()
                                       ->get_input_layouts(); // 保持しなければ名前が消える
        const auto input_elements = input_layouts |
                                    std::views::transform([](const types::ShaderInputLayout& info) {
                                        return D3D11Converter::to_input_element_description(info);
                                    }) |
                                    std::ranges::to<std::vector>();

        const auto [resource_handle, input_layout] =
            this->native_resource->get_native_input_layout_accessor()->make_native_input_layout();
        const auto device = this->context->get_device();
        const HRESULT hr = device->CreateInputLayout(input_elements.data(),
            static_cast<uint32_t>(input_elements.size()),
            shader_data.code.data(),
            shader_data.code.size(),
            input_layout.GetAddressOf());
        if (FAILED(hr)) {
            return foundation::Error(
                platform::RenderError::MakeError, "InputLayoutの作成に失敗しました");
        }

        return this->handle_mapper->make(types::RenderHandleType::VertexLayout,
            types::ResourceHandles{
                .resource = resource_handle,
            });
    }

    foundation::Result<types::RenderHandle, platform::RenderError> ResourceManager::make_mesh(
        types::MeshData&& mesh_data, const std::vector<types::RenderHandle>& shader_reflections) {
        auto [resource_handle, mesh] =
            this->native_resource->get_mesh_accessor()->make_mesh_handles();

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
        types::HandleId mapped_index_buffer = types::HandleId{};
        {
            auto&& result = this->make_index_buffer(mesh_data.indices.get_render_data())
                                .add_message("インデックスバッファの作成に失敗しました");
            if (result.is_err()) {
                return std::move(result);
            }
            const auto& index_handle = mesh.mesh_handles.emplace_back(result.unwrap());

            const auto opt_mapped = this->handle_mapper->get(index_handle);
            if (opt_mapped.is_none()) {
                return foundation::Error(
                    platform::RenderError::MakeError, "バインド情報の取得に失敗しました");
            }
            const auto& mapped = opt_mapped.unwrap();
            mapped_index_buffer = mapped.resource;
        }

        // マテリアル情報をもとにシェーダーリフレクションから
        // 定数バッファやサンプラーなどのバインド位置を取得
        auto&& result = this->resolve_mesh_binding(std::move(mesh_data),
                                this->get_shader_reflections(shader_reflections),
                                std::move(mapped_index_buffer))
                            .add_message("バインド情報の取得に失敗しました");
        if (result.is_err()) {
            return result.propagation(platform::RenderError::MakeError);
        }
        mesh.mesh_handles.append_range(std::move(result).unwrap_mut());

        return this->handle_mapper->make(types::RenderHandleType::Mesh,
            types::ResourceHandles{
                .resource = resource_handle,
            });
    }

    foundation::Result<types::RenderHandle, platform::RenderError> ResourceManager::make_shader(
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

        return foundation::Error(platform::RenderError::MakeError);
    }

    foundation::Result<types::RenderHandle, platform::RenderError> ResourceManager::make_texture(
        const types::TextureData& texture_data) {
        auto&& result_image =
            this->make_image(texture_data).add_message("イメージの作成に失敗しました");
        if (result_image.is_err()) {
            return result_image;
        }
        auto& image_handle = result_image.unwrap();
        auto&& reuslt_srv = this->make_view(image_handle,
            types::ImageViewDescription::make_shader_resource_view_description(
                types::ImageFormat::D24_UNORM_S8_UINT));
        if (reuslt_srv.is_err()) {
            return foundation::Error(
                platform::RenderError::MakeError, "シェーダーリソースビューの作成に失敗しました");
        }

        return reuslt_srv;
    }

    foundation::Result<types::RenderHandle, platform::RenderError>
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
        const auto [resource_handle, texture] = buffer_accessor->make_native_texture_2d();
        const auto device = this->context->get_device();
        const HRESULT hr = device->CreateTexture2D(&desc, &subresource, texture.GetAddressOf());
        if (FAILED(hr)) {
            return foundation::Error(
                platform::RenderError::MakeError, "テクスチャの作成に失敗しました");
        }

        const auto [binding_index, binding] = this->resource_binder->make_image_binding();

        return this->handle_mapper->make(types::RenderHandleType::Image,
            types::ResourceHandles{
                .resource = resource_handle,
                .binding = binding_index,
            });
    }

    foundation::Result<types::RenderHandle, platform::RenderError>
    ResourceManager::make_vertex_buffer(const types::RenderData& data) {
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
        const auto [resource_handle, buffer] = buffer_accessor->make_native_buffer();
        const auto device = this->context->get_device();
        const HRESULT hr = device->CreateBuffer(&desc, &init_data, buffer.GetAddressOf());
        if FAILED (hr) {
            return foundation::Error(
                platform::RenderError::MakeError, "バッファの作成に失敗しました");
        }

        const auto [binding_index, binding] = this->resource_binder->make_buffer_binding();
        binding.parameter = types::VertexBufferParameter{
            .target = 0, // 仮
            .stride = data.stride,
            .offset = 0,
        };

        return this->handle_mapper->make(types::RenderHandleType::Buffer,
            types::ResourceHandles{
                .resource = resource_handle,
                .binding = binding_index,
            });
    }

    foundation::Result<types::RenderHandle, platform::RenderError>
    ResourceManager::make_index_buffer(const types::RenderData& data) {
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
        const auto [resource_handle, buffer] = buffer_accessor->make_native_buffer();
        const auto device = this->context->get_device();
        const HRESULT hr = device->CreateBuffer(&desc, &init_data, buffer.GetAddressOf());
        if FAILED (hr) {
            return foundation::Error(
                platform::RenderError::MakeError, "バッファの作成に失敗しました");
        }

        const auto [binding_index, binding] = this->resource_binder->make_buffer_binding();
        binding.parameter = types::IndexBufferParameter{
            .stride = data.stride,
        };

        return this->handle_mapper->make(types::RenderHandleType::Buffer,
            types::ResourceHandles{
                .resource = resource_handle,
                .binding = binding_index,
            });
    }

    foundation::Result<types::RenderHandle, platform::RenderError>
    ResourceManager::make_uniform_buffer(const types::RenderData& data) {
        const D3D11_BUFFER_DESC desc{
            .ByteWidth = static_cast<UINT>(data.byte_width()),
            .Usage = D3D11_USAGE_DYNAMIC,
            .BindFlags = D3D11_BIND_CONSTANT_BUFFER,
            .CPUAccessFlags = D3D11_CPU_ACCESS_WRITE,
        };
        const D3D11_SUBRESOURCE_DATA init_data{
            .pSysMem = data.raw_data(),
            .SysMemPitch = 0,
            .SysMemSlicePitch = data.stride,
        };

        const auto buffer_accessor = this->native_resource->get_native_buffer_accessor();
        const auto [resource_handle, buffer] = buffer_accessor->make_native_buffer();
        const auto device = this->context->get_device();
        const HRESULT hr = device->CreateBuffer(&desc, &init_data, buffer.GetAddressOf());
        if (FAILED(hr)) {
            return foundation::Error(
                platform::RenderError::MakeError, "バッファの作成に失敗しました");
        }

        const auto [binding_index, binding] = this->resource_binder->make_buffer_binding();
        binding.parameter = types::UniformBufferParameter{};

        const auto [config_index, config] =
            this->native_resource->get_buffer_accessor()->make_buffer();

        return this->handle_mapper->make(types::RenderHandleType::Buffer,
            types::ResourceHandles{
                .resource = resource_handle,
                .binding = binding_index,
                .configurable = config_index,
            });
    }

    foundation::Result<types::RenderHandle, platform::RenderError> ResourceManager::make_image(
        const types::ImageDescription& description) {
        // 先に作成
        const auto buffer_accessor = this->native_resource->get_native_texture_accessor();
        const auto [resource_handle, texture] = buffer_accessor->make_native_texture_2d();
        const auto desc = D3D11Converter::to_texture2d_desc(description);

        if (description.contains(types::ImageUsage::BackBuffer)) {
            const auto swap_chain = this->context->get_swap_chain();
            const HRESULT hr = swap_chain->GetBuffer(0, IID_PPV_ARGS(texture.GetAddressOf()));
            if (FAILED(hr)) {
                return foundation::Error(
                    platform::RenderError::MakeError, "バックバッファの取得に失敗しました");
            }
        } else {
            const auto device = this->context->get_device();
            const HRESULT hr = device->CreateTexture2D(&desc, nullptr, texture.GetAddressOf());
            if (FAILED(hr)) {
                return foundation::Error(
                    platform::RenderError::MakeError, "イメージの作成に失敗しました");
            }
        }

        const auto [binding_index, binding] = this->resource_binder->make_image_binding();

        return this->handle_mapper->make(types::RenderHandleType::Image,
            types::ResourceHandles{
                .resource = resource_handle,
                .binding = binding_index,
            });
    }

    foundation::Result<types::RenderHandle, platform::RenderError> ResourceManager::make_image(
        const types::TextureData& texture_data) {
        // 先に作成
        const auto buffer_accessor = this->native_resource->get_native_texture_accessor();
        const auto [resource_handle, texture] = buffer_accessor->make_native_texture_2d();
        const auto desc = D3D11Converter::to_texture2d_desc(texture_data);

        if (texture_data.mips.empty()) {
            return foundation::Error(
                platform::RenderError::MakeError, "イメージの作成に失敗しました");
        }

        // 初期データの作成
        const auto mip_index = 0;
        const auto& mip = texture_data.mips[mip_index];
        const D3D11_SUBRESOURCE_DATA init_subresource{
            .pSysMem = mip.pixels.data(),
            .SysMemPitch = mip.row_pitch,
            .SysMemSlicePitch = mip.slice_pitch,
        };

        const auto device = this->context->get_device();
        const HRESULT hr =
            device->CreateTexture2D(&desc, &init_subresource, texture.GetAddressOf());
        if (FAILED(hr)) {
            return foundation::Error(
                platform::RenderError::MakeError, "イメージの作成に失敗しました");
        }

        const auto [binding_index, binding] = this->resource_binder->make_image_binding();

        return this->handle_mapper->make(types::RenderHandleType::Image,
            types::ResourceHandles{
                .resource = resource_handle,
                .binding = binding_index,
            });
    }

    foundation::Result<types::RenderHandle, platform::RenderError>
    ResourceManager::make_blend_state(const types::BlendStateDescription& description) {
        const auto [resource_handle, satate] =
            this->native_resource->get_native_state_accessor()->make_native_blend_state();
        const auto desc = D3D11Converter::to_blend_desc(description);
        const auto device = this->context->get_device();
        const HRESULT hr = device->CreateBlendState(&desc, satate.GetAddressOf());
        if (FAILED(hr)) {
            return foundation::Error(
                platform::RenderError::MakeError, "ブレンドステートの作成に失敗しました");
        }

        const auto [binding_index, binding] = this->resource_binder->make_state_binding();
        binding.parameter = types::BlendStateParameter{};

        return this->handle_mapper->make(types::RenderHandleType::State,
            types::ResourceHandles{
                .resource = resource_handle,
                .binding = binding_index,
            });
    }

    foundation::Result<types::RenderHandle, platform::RenderError>
    ResourceManager::make_sampler_state(const types::SamplerStateDescription& description) {
        const auto [resource_handle, satate] =
            this->native_resource->get_native_state_accessor()->make_native_sampler_state();
        const auto desc = D3D11Converter::to_sampler_desc(description);
        const auto device = this->context->get_device();
        const HRESULT hr = device->CreateSamplerState(&desc, satate.GetAddressOf());
        if (FAILED(hr)) {
            return foundation::Error(
                platform::RenderError::MakeError, "サンプラーステートの作成に失敗しました");
        }

        const auto [binding_index, binding] = this->resource_binder->make_state_binding();
        binding.parameter = types::SamplerStateParameter{};

        return this->handle_mapper->make(types::RenderHandleType::State,
            types::ResourceHandles{
                .resource = resource_handle,
                .binding = binding_index,
            });
    }

    foundation::Result<types::RenderHandle, platform::RenderError>
    ResourceManager::make_rasterizer_state(const types::RasterizerStateDescription& description) {
        const auto desc = D3D11Converter::to_rasterizer_desc(description);

        const auto [resource_handle, rasterizer] =
            this->native_resource->get_native_state_accessor()->make_native_rasterizer_state();
        const auto device = this->context->get_device();
        const HRESULT hr = device->CreateRasterizerState(&desc, rasterizer.GetAddressOf());
        if (FAILED(hr)) {
            return foundation::Error(
                platform::RenderError::MakeError, "ラスタライザの作成に失敗しました");
        }

        const auto [binding_index, binding] = this->resource_binder->make_state_binding();
        binding.parameter = types::RasterizerStateParameter{};

        return this->handle_mapper->make(types::RenderHandleType::State,
            types::ResourceHandles{
                .resource = resource_handle,
                .binding = binding_index,
            });
    }

    foundation::Result<types::RenderHandle, platform::RenderError>
    ResourceManager::make_depth_stencil_state(
        const types::DepthStencilStateDescription& description) {
        const auto [resource_handle, satate] =
            this->native_resource->get_native_state_accessor()->make_native_depth_stencil_state();
        const auto desc = D3D11Converter::to_depth_stencil_desc(description);
        const auto device = this->context->get_device();
        const HRESULT hr = device->CreateDepthStencilState(&desc, satate.GetAddressOf());
        if (FAILED(hr)) {
            return foundation::Error(
                platform::RenderError::MakeError, "深度ステンシルステートの作成に失敗しました");
        }

        const auto [binding_index, binding] = this->resource_binder->make_state_binding();
        binding.parameter = types::DepthStencilStateParameter{};

        return this->handle_mapper->make(types::RenderHandleType::State,
            types::ResourceHandles{
                .resource = resource_handle,
                .binding = binding_index,
            });
    }

    foundation::Result<types::RenderHandle, platform::RenderError> ResourceManager::make_view(
        const types::RenderHandle& image_handle, const types::ImageViewDescription& description) {
        // テクスチャから
        auto opt_index = this->handle_mapper->get(image_handle);
        if (opt_index.is_none()) {
            return foundation::Error(
                platform::RenderError::MakeError, "イメージが見つかりませんでした");
        }
        const auto& index = opt_index.unwrap();

        switch (description.type) {
            case types::ImageViewType::DepthStencil:
                return this->make_depth_stencil_view(index, description);
            case types::ImageViewType::RenderTarget:
                return this->make_render_target_view(index, description);
            case types::ImageViewType::ShaderResource:
                return this->make_shader_resource_view(index, description);
            case types::ImageViewType::UnorderedAccess:
                return this->make_unodered_access_view(index, description);
            default:
                break;
        }

        return foundation::Error(
            platform::RenderError::MakeError, "対応していないフォーマットです");
    }

    foundation::Result<types::RenderHandle, platform::RenderError> ResourceManager::make_viewport(
        const types::ViewportRect& config) {
        const auto viewport_accessor = this->native_resource->get_native_viewport_accessor();
        const auto [resource_handle, viewport] = viewport_accessor->make_native_viewport();

        viewport.TopLeftX = static_cast<FLOAT>(config.left_top_x);
        viewport.TopLeftY = static_cast<FLOAT>(config.left_top_y);
        viewport.Width = static_cast<FLOAT>(config.width);
        viewport.Height = static_cast<FLOAT>(config.height);
        viewport.MinDepth = static_cast<FLOAT>(config.min_depth);
        viewport.MaxDepth = static_cast<FLOAT>(config.max_depth);

        return this->handle_mapper->make(types::RenderHandleType::ViewPort,
            types::ResourceHandles{
                .resource = resource_handle,
            });
    }

    platform::IGPUResourceAccessor* ResourceManager::get_resource_accessor(void) noexcept {
        return this->native_resource.get();
    }

    const platform::IGPUResourceAccessor* ResourceManager::get_resource_accessor(
        void) const noexcept {
        return this->native_resource.get();
    }

    foundation::Result<types::RenderHandle, platform::RenderError>
    ResourceManager::make_render_target_view(
        const types::ResourceHandles image_index, const types::ImageViewDescription& description) {
        const auto texture_accessor = this->native_resource->get_native_texture_accessor();
        const auto opt_texture = texture_accessor->get_native_texture_2d(image_index.resource);
        if (opt_texture.is_none()) {
            return foundation::Error(
                platform::RenderError::MakeError, "イメージが見つかりませんでした");
        }
        const auto& texture = opt_texture.unwrap();

        // RenderTargetViewの作成
        const auto [resource_handle, rtv] =
            this->native_resource->get_native_view_accessor()->make_native_render_target_view();
        const auto device = this->context->get_device();
        const HRESULT hr =
            device->CreateRenderTargetView(texture.Get(), nullptr, rtv.GetAddressOf());
        if (FAILED(hr)) {
            return foundation::Error(
                platform::RenderError::MakeError, "レンダーターゲットの作成に失敗しました");
        }

        // バインド時のパラメータ用
        const auto [binding_index, binding] = this->resource_binder->make_view_binding();
        binding.parameter = types::RenderTargetParameter{};

        const auto render_handle = this->handle_mapper->make(types::RenderHandleType::View,
            types::ResourceHandles{
                .resource = resource_handle,
                .binding = binding_index,
            });

        // 外部変更用のビューの作成
        const auto configurable_index =
            this->native_resource->get_view_accessor()->make_render_target_view(resource_handle,
                std::make_shared<D3D11RenderTargetView>(render_handle, description));
        auto opt_mapped = this->handle_mapper->get(render_handle);
        if (opt_mapped.is_none()) {
            return foundation::Error(
                platform::RenderError::MakeError, "バインド情報が見つかりませんでした");
        }
        auto& mapped = opt_mapped.unwrap_mut();
        mapped.configurable = configurable_index;

        return render_handle;
    }

    foundation::Result<types::RenderHandle, platform::RenderError>
    ResourceManager::make_depth_stencil_view(
        const types::ResourceHandles image_index, const types::ImageViewDescription& description) {
        const auto texture_accessor = this->native_resource->get_native_texture_accessor();
        const auto opt_texture = texture_accessor->get_native_texture_2d(image_index.resource);
        if (opt_texture.is_none()) {
            return foundation::Error(
                platform::RenderError::MakeError, "イメージが見つかりませんでした");
        }
        const auto& texture = opt_texture.unwrap();

        // DepthStencilViewの作成
        const auto [resource_handle, dsv] =
            this->native_resource->get_native_view_accessor()->make_native_depth_stencil_view();
        const auto device = this->context->get_device();
        const HRESULT hr =
            device->CreateDepthStencilView(texture.Get(), nullptr, dsv.GetAddressOf());
        if (FAILED(hr)) {
            return foundation::Error(
                platform::RenderError::MakeError, "深度ステンシルの作成に失敗しました");
        }

        // バインド時のパラメータ用
        const auto [binding_index, binding] = this->resource_binder->make_view_binding();
        binding.parameter = types::DepthStencilParameter{};

        const auto render_handle = this->handle_mapper->make(types::RenderHandleType::View,
            types::ResourceHandles{
                .resource = resource_handle,
                .binding = binding_index,
            });

        // 外部変更用のビューの作成
        const auto configurable_index =
            this->native_resource->get_view_accessor()->make_depth_stencil_view(resource_handle,
                std::make_shared<D3D11DepthStencilView>(render_handle, description));
        auto opt_mapped = this->handle_mapper->get(render_handle);
        if (opt_mapped.is_none()) {
            return foundation::Error(
                platform::RenderError::MakeError, "バインド情報が見つかりませんでした");
        }
        auto& mapped = opt_mapped.unwrap_mut();
        mapped.configurable = configurable_index;

        return render_handle;
    }

    foundation::Result<types::RenderHandle, platform::RenderError>
    ResourceManager::make_shader_resource_view(
        const types::ResourceHandles image_index, const types::ImageViewDescription& description) {
        const auto texture_accessor = this->native_resource->get_native_texture_accessor();
        const auto opt_texture = texture_accessor->get_native_texture_2d(image_index.resource);
        if (opt_texture.is_none()) {
            return foundation::Error(
                platform::RenderError::MakeError, "イメージが見つかりませんでした");
        }
        const auto& texture = opt_texture.unwrap();

        // ShaderResourceViewの作成
        const auto [resource_handle, srv] =
            this->native_resource->get_native_view_accessor()->make_native_shader_resource_view();
        const auto device = this->context->get_device();
        const HRESULT hr =
            device->CreateShaderResourceView(texture.Get(), nullptr, srv.GetAddressOf());
        if (FAILED(hr)) {
            return foundation::Error(
                platform::RenderError::MakeError, "シェーダーリソースの作成に失敗しました");
        }

        // バインド時のパラメータ用
        const auto [binding_index, binding] = this->resource_binder->make_view_binding();
        binding.parameter = types::ShaderResourceParameter{};

        const auto render_handle = this->handle_mapper->make(types::RenderHandleType::View,
            types::ResourceHandles{
                .resource = resource_handle,
                .binding = binding_index,
            });

        // 外部変更用のビューの作成
        const auto configurable_index =
            this->native_resource->get_view_accessor()->make_shader_resource_view(resource_handle,
                std::make_shared<D3D11ShaderResourceView>(render_handle, description));
        auto opt_mapped = this->handle_mapper->get(render_handle);
        if (opt_mapped.is_none()) {
            return foundation::Error(
                platform::RenderError::MakeError, "バインド情報が見つかりませんでした");
        }
        auto& mapped = opt_mapped.unwrap_mut();
        mapped.configurable = configurable_index;

        return render_handle;
    }

    foundation::Result<types::RenderHandle, platform::RenderError>
    ResourceManager::make_unodered_access_view(
        const types::ResourceHandles image_index, const types::ImageViewDescription& description) {
        const auto texture_accessor = this->native_resource->get_native_texture_accessor();
        const auto opt_texture = texture_accessor->get_native_texture_2d(image_index.resource);
        if (opt_texture.is_none()) {
            return foundation::Error(
                platform::RenderError::MakeError, "イメージが見つかりませんでした");
        }
        const auto& texture = opt_texture.unwrap();

        // UnorderedAccessViewの作成
        const auto [resource_handle, uav] =
            this->native_resource->get_native_view_accessor()->make_native_unordered_access_view();
        const auto device = this->context->get_device();
        const HRESULT hr =
            device->CreateUnorderedAccessView(texture.Get(), nullptr, uav.GetAddressOf());
        if (FAILED(hr)) {
            return foundation::Error(
                platform::RenderError::MakeError, "シェーダーリソースの作成に失敗しました");
        }

        // バインド時のパラメータ用
        const auto [binding_index, binding] = this->resource_binder->make_view_binding();
        binding.parameter = types::UnorderedAccessParameter{};

        const auto render_handle = this->handle_mapper->make(types::RenderHandleType::View,
            types::ResourceHandles{
                .resource = resource_handle,
                .binding = binding_index,
            });

        // 外部変更用のビューの作成
        const auto configurable_index =
            this->native_resource->get_view_accessor()->make_unodered_access_view(resource_handle,
                std::make_shared<D3D11UnorderedAccessView>(render_handle, description));
        auto opt_mapped = this->handle_mapper->get(render_handle);
        if (opt_mapped.is_none()) {
            return foundation::Error(
                platform::RenderError::MakeError, "バインド情報が見つかりませんでした");
        }
        auto& mapped = opt_mapped.unwrap_mut();
        mapped.configurable = configurable_index;

        return render_handle;
    }

    foundation::Result<types::RenderHandle, platform::RenderError>
    ResourceManager::make_shader_from_dxbc(
        const types::ShaderKind kind, const types::ShaderData& shader_data) {
        const auto shader_accessor = this->native_resource->get_native_shader_accessor();
        const auto device = this->context->get_device();

        types::HandleId resource_handle;
        types::HandleId binding_handle;
        switch (kind) {
            case types::ShaderKind::Vertex: {
                const auto [resource, shader] = shader_accessor->make_native_vertex_shader();
                const HRESULT hr = device->CreateVertexShader(shader_data.code.data(),
                    shader_data.code.size(),
                    nullptr,
                    shader.GetAddressOf());
                if (FAILED(hr)) {
                    return foundation::Error(platform::RenderError::MakeError);
                }
                const auto [binding, _] = this->resource_binder->make_shader_binding();

                binding_handle = binding;
                resource_handle = resource;
            } break;
            case types::ShaderKind::Pixel: {
                const auto [resource, shader] = shader_accessor->make_native_pixel_shader();
                const HRESULT hr = device->CreatePixelShader(shader_data.code.data(),
                    shader_data.code.size(),
                    nullptr,
                    shader.GetAddressOf());
                if (FAILED(hr)) {
                    return foundation::Error(platform::RenderError::MakeError);
                }
                const auto [binding, _] = this->resource_binder->make_shader_binding();

                binding_handle = binding;
                resource_handle = resource;
            } break;
            case types::ShaderKind::Compute: {
                const auto [resource, shader] = shader_accessor->make_native_compute_shader();
                const HRESULT hr = device->CreateComputeShader(shader_data.code.data(),
                    shader_data.code.size(),
                    nullptr,
                    shader.GetAddressOf());
                if (FAILED(hr)) {
                    return foundation::Error(platform::RenderError::MakeError);
                }
                const auto [binding, _] = this->resource_binder->make_shader_binding();

                binding_handle = binding;
                resource_handle = resource;
            } break;
            case types::ShaderKind::Hull: {
                const auto [resource, shader] = shader_accessor->make_native_hull_shader();
                const HRESULT hr = device->CreateHullShader(shader_data.code.data(),
                    shader_data.code.size(),
                    nullptr,
                    shader.GetAddressOf());
                if (FAILED(hr)) {
                    return foundation::Error(platform::RenderError::MakeError);
                }
                const auto [binding, _] = this->resource_binder->make_shader_binding();

                binding_handle = binding;
                resource_handle = resource;
            } break;
            default:
                return foundation::Error(platform::RenderError::MakeError);
        }

        return this->handle_mapper->make(types::RenderHandleType::Shader,
            types::ResourceHandles{
                .resource = resource_handle,
                .binding = binding_handle,
            });
    }

    foundation::Result<std::vector<types::RenderHandle>, platform::RenderError>
    ResourceManager::resolve_mesh_binding(types::MeshData&& mesh_data,
        std::vector<platform::IShaderAccessor::ShaderReflection>&& shader_reflections,
        types::HandleId&& mapped_index_buffer) {
        std::vector<types::RenderHandle> mesh_handles;

        auto&& result_uniforms =
            this->resolve_uniforms(std::move(mesh_data.uniforms), shader_reflections)
                .add_message("Unifromバッファのバインド情報の解決に失敗しました");
        if (result_uniforms.is_err()) {
            return result_uniforms;
        }
        mesh_handles.append_range(std::move(result_uniforms).unwrap_mut());

        // サンプラーとテクスチャの作成
        for (auto& material : mesh_data.materials) {
            auto&& result_textures =
                this->resolve_texture(material, shader_reflections)
                    .add_message("テクスチャのバインド情報の解決に失敗しました");
            if (result_textures.is_err()) {
                return result_textures;
            }
            mesh_handles.append_range(std::move(result_textures).unwrap_mut());

            // 描画命令は最後
            auto [binding_handle, bind] = this->resource_binder->make_draw_binding();
            bind = std::move(material.draw_binding);

            mesh_handles.emplace_back(this->handle_mapper->make(types::RenderHandleType::Draw,
                types::ResourceHandles{
                    .resource = mapped_index_buffer,
                    .binding = binding_handle,
                }));
        }

        return mesh_handles;
    }

    foundation::Result<std::vector<types::RenderHandle>, platform::RenderError>
    ResourceManager::resolve_uniforms(types::MeshData::UniformMap&& uniforms,
        const std::vector<platform::IShaderAccessor::ShaderReflection>& shader_reflections) {
        std::vector<types::RenderHandle> mesh_handles;

        // 定数バッファの作成
        for (const auto& shader_reflection : shader_reflections) {
            const auto& shader_kind = shader_reflection->get_shader_kind();
            const auto& input_reflection = shader_reflection->get_shader_input_reflection();
            for (auto iter = uniforms.begin(); iter != uniforms.end();) {
                auto& [name, owned_render_data] = *iter;
                const auto opt_input_resource = input_reflection->resolve_input_resource(name);

                if (opt_input_resource.is_some()) {
                    auto&& result = this->resolve_uniform(
                        opt_input_resource.unwrap(), shader_kind, std::move(owned_render_data));
                    if (result.is_err()) {
                        return std::move(result).unwrap_err();
                    }

                    mesh_handles.emplace_back(result.unwrap());

                    iter = uniforms.erase(iter);
                } else {
                    iter++;
                }
            }
        }

        if (!uniforms.empty()) {
            foundation::StringBuilder strings;
            strings.push_back("解決できないデータが見つかりました");
            for (const auto& [name, data] : uniforms) {
                auto render_data = data.get_render_data();
                strings.push_back(
                    std::format("[未解決の定数バッファ] name: {}, bytes: {}, stride: {}",
                        name,
                        render_data.bytes.size(),
                        render_data.stride));
            }
            // エラーで落とすか警告かどっちがいいんだろうね
            // return foundation::Error(platform::RenderError::BufferError, strings.join("\n"));
            foundation::Logger::warning(strings.join("\n"));
        }

        return mesh_handles;
    }

    foundation::Result<types::RenderHandle, platform::RenderError> ResourceManager::resolve_uniform(
        const types::ShaderInputResource& input_resource,
        const types::ShaderKind& shader_kind,
        types::OwnedRenderData&& render_data) {
        auto&& result = this->make_uniform_buffer(render_data.get_render_data())
                            .add_message("定数バッファの作成に失敗しました");
        if (result.is_err()) {
            return result.propagation(platform::RenderError::MakeError);
        }

        const auto& handle = result.unwrap();
        const auto opt_index = this->handle_mapper->get(handle);
        if (opt_index.is_none()) {
            return foundation::Error(
                platform::RenderError::MakeError, "リソースが取得できませんでした");
        }
        const auto& index = opt_index.unwrap();

        // バインド情報の更新
        auto opt_binding = this->resource_binder->get_buffer_binding(index.binding);
        if (opt_binding.is_none()) {
            return foundation::Error(platform::RenderError::MakeError);
        }
        auto& binding = opt_binding.unwrap_mut();
        if (auto param = std::get_if<types::UniformBufferParameter>(&binding.parameter)) {
            param->target_shader = shader_kind;
            param->target = input_resource.binding;
        } else {
            return foundation::Error(platform::RenderError::MakeError);
        }

        // 名前に対応したUniformBufferのUpderterの作成
        const auto opt_native_buffer =
            this->native_resource->get_native_buffer_accessor()->get_native_buffer(index.resource);
        if (opt_native_buffer.is_none()) {
            return foundation::Error(
                platform::RenderError::MakeError, "リソースが取得できませんでした");
        }
        const auto& native_buffer = opt_native_buffer.unwrap();

        // 外部変更用
        auto opt_buffer =
            this->native_resource->get_buffer_accessor()->get_buffer(index.configurable);
        if (opt_buffer.is_none()) {
            return foundation::Error(
                platform::RenderError::MakeError, "リソースが取得できませんでした");
        }
        auto& buffer = opt_buffer.unwrap_mut();
        buffer = BufferInterfaceMaker::make_uniform(
            std::move(render_data), this->context->get_context(), native_buffer);

        return handle;
    }

    foundation::Result<std::vector<types::RenderHandle>, platform::RenderError>
    ResourceManager::resolve_texture(const types::MeshMaterial& mesh_material,
        const std::vector<platform::IShaderAccessor::ShaderReflection>& shader_reflections) {
        std::vector<types::RenderHandle> mesh_handles;
        mesh_handles.reserve(mesh_material.textures.size());

        // マテリアルから
        for (const auto& [target_name, texture] : mesh_material.textures) {
            if (texture->mips.empty()) {
                return foundation::Error(
                    platform::RenderError::MakeError, "テクスチャデータが存在しません");
            }
            const auto pre_size = mesh_handles.size();

            // どのシェーダーが所持しているのかわからないので全探索(TODO: 全探索以外の方法を探す)
            for (const auto& shader_reflection : shader_reflections) {
                auto result = this->resolve_texture(shader_reflection, target_name, texture);
                if (result.is_err()) {
                    return std::move(result).unwrap_err();
                }
                auto& opt_handle = result.unwrap();
                if (opt_handle.is_none()) {
                    continue;
                }

                mesh_handles.emplace_back(opt_handle.unwrap());
                break;
            }

            if (pre_size == mesh_handles.size()) {
                foundation::Logger::warning(
                    std::format("[未解決のテクスチャ] name: {}, height: {}, width: {}",
                        target_name,
                        texture->height,
                        texture->width));
            }
        }

        return mesh_handles;
    }

    foundation::Result<foundation::Option<types::RenderHandle>, platform::RenderError>
    ResourceManager::resolve_texture(
        const platform::IShaderAccessor::ShaderReflection& shader_reflection,
        const foundation::UTF8& target_name,
        const std::shared_ptr<types::TextureData>& texture) {
        const auto& shader_kind = shader_reflection->get_shader_kind();
        const auto& input_reflection = shader_reflection->get_shader_input_reflection();
        const auto opt_input_resource = input_reflection->resolve_input_resource(target_name);
        if (opt_input_resource.is_none()) {
            return foundation::Option<types::RenderHandle>{};
        }

        const auto& input_resource = opt_input_resource.unwrap();
        switch (input_resource.type) {
            case types::ShaderInputResourceType::Texture: {
                auto&& result =
                    this->make_texture(*texture).add_message("テクスチャ生成に失敗しました");
                if (result.is_err()) {
                    return result;
                }
                const auto& handle = result.unwrap();

                // バインド情報の更新
                const auto& binding_handle = (*this->handle_mapper)[handle].binding;
                auto opt_binding = this->resource_binder->get_view_binding(binding_handle);
                if (opt_binding.is_none()) {
                    return foundation::Option<types::RenderHandle>{};
                }
                auto& binding = opt_binding.unwrap_mut();
                auto srv_binding = std::get_if<types::ShaderResourceParameter>(&binding.parameter);
                if (!bool(srv_binding)) {
                    return foundation::Option<types::RenderHandle>{};
                }
                srv_binding->target_shader = shader_kind;
                srv_binding->target = input_resource.binding;

                return handle;
            } break;
            case types::ShaderInputResourceType::Sampler: {
                // TODO
                auto&& result =
                    this->make_sampler_state(types::SamplerStateDescription::default_linear())
                        .add_message("サンプラーステートの作成に失敗しました");
                if (result.is_err()) {
                    return std::move(result).unwrap_err();
                }

                // バインド情報の更新
                const auto& handle = result.unwrap();
                const auto& binding_handle = (*this->handle_mapper)[handle].binding;
                auto opt_binding = this->resource_binder->get_state_binding(binding_handle);
                if (opt_binding.is_none()) {
                    return foundation::Option<types::RenderHandle>{};
                }
                auto& binding = opt_binding.unwrap_mut();
                auto sampler_binding =
                    std::get_if<types::SamplerStateParameter>(&binding.parameter);
                if (!bool(sampler_binding)) {
                    return foundation::Option<types::RenderHandle>{};
                }
                sampler_binding->target_shader = shader_kind;
                sampler_binding->target = input_resource.binding;

                return handle;
            } break;
            default:
                break;
        }

        return foundation::Option<types::RenderHandle>{};
    }

    std::vector<platform::IShaderAccessor::ShaderReflection>
    ResourceManager::get_shader_reflections(
        const std::vector<types::RenderHandle>& shader_reflections) const {
        using ReturnType = foundation::Option<platform::IShaderAccessor::ShaderReflection>;
        return shader_reflections |
               std::views::transform([this](const types::RenderHandle& handle) -> ReturnType {
                   const auto opt_index = this->handle_mapper->get(handle);
                   if (opt_index.is_none()) {
                       return {};
                   }
                   const auto& index = opt_index.unwrap();
                   auto opt_refection =
                       this->native_resource->get_shader_accessor()->get_shader_reflection(
                           index.configurable);
                   if (opt_refection.is_none()) {
                       return {};
                   }
                   return opt_refection.unwrap();
               }) |
               std::views::filter(
                   [](const ReturnType& reflection) { return reflection.is_some(); }) |
               std::views::transform(
                   [this](const ReturnType& opt_reflection) { return opt_reflection.unwrap(); }) |
               std::ranges::to<std::vector>();
    }
} // namespace enishi::renderer::directx