#include "d3d11_renderer.h"
#include "d3d11_converter.h"
#include "view/render_target_view.h"
#include <ranges>

namespace enishi::renderer::directx {
    D3D11Renderer::D3D11Renderer(std::unique_ptr<D3D11> d3d11)
        : d3d11(std::move(d3d11)) {
        this->resource_manager = std::make_unique<ResourceManager>(this->d3d11);
    }

    platform::RenderResult<types::RenderHandle> D3D11Renderer::create_viewport(
        const types::ViewportRect& config) {
        const auto result = this->resource_manager->make_viewport(config);
        if (result.is_err()) {
            return result.propagation(platform::RenderError::MakeError);
        }
        return result.unwrap();
    }

    platform::RenderResult<std::unique_ptr<platform::IPipelineLayout>>
    D3D11Renderer::create_vertex_layout(const types::VertexLayout& layout,
        const types::RenderHandle& vertex_shader,
        const types::RenderHandle& pixel_shader) {
        return std::unique_ptr<platform::IPipelineLayout>{};
    }

    platform::RenderResult<types::RenderHandle> D3D11Renderer::create_shader_reflection(
        const types::ShaderData& shader_data) {
        const auto result = this->resource_manager->make_shader_reflection(shader_data);
        if (result.is_err()) {
            return result.propagation(platform::RenderError::MakeError);
        }

        return result.unwrap();
    }

    platform::RenderResult<types::RenderHandle>
    D3D11Renderer::create_vertex_layout_from_shader_data(const types::ShaderData& shader_data) {
        const auto result = this->resource_manager->make_input_layout_from_shader_data(shader_data)
                                .add_message("シェーダーから頂点レイアウトを作成できませんでした");
        if (result.is_err()) {
            return result.propagation(platform::RenderError::MakeError);
        }

        return result.unwrap();
    }

    platform::RenderResult<types::RenderHandle> D3D11Renderer::create_rasterizer(
        const types::RasterizerDescription& description) {
        const auto result = this->resource_manager->make_rasterizer(description);
        if (result.is_err()) {
            return result.propagation(platform::RenderError::MakeError);
        }

        return result.unwrap();
    }

    platform::RenderResult<types::RenderHandle> D3D11Renderer::create_image(
        const types::ImageDescription& description) {
        const auto result = this->resource_manager->make_image(description);
        if (result.is_err()) {
            return result.propagation(platform::RenderError::MakeError);
        }

        return result.unwrap();
    }

    platform::RenderResult<std::shared_ptr<platform::IRenderTargetView>>
    D3D11Renderer::create_render_target_view(
        types::RenderHandle image_handle, const types::ImageViewDescription& description) {
        const auto result =
            this->resource_manager->make_render_target_view(image_handle, description);
        if (result.is_err()) {
            return result.propagation(platform::RenderError::MakeError);
        }
        auto handle = result.unwrap();
        auto opt_rtv = this->resource_manager->get_accessor()->get_render_target(handle.id);
        if (opt_rtv.is_none()) {
            return foundation::Error(platform::RenderError::MakeError);
        }

        return opt_rtv.unwrap();
    }

    platform::RenderResult<std::shared_ptr<platform::IDepthStencilView>>
    D3D11Renderer::create_depth_stencil_view(
        types::RenderHandle image_handle, const types::ImageViewDescription& description) {
        const auto result = this->resource_manager->make_depth_(image_handle, description);
        if (result.is_err()) {
            return result.propagation(platform::RenderError::MakeError);
        }
        auto handle = result.unwrap();
        auto opt_rtv = this->resource_manager->get_accessor()->get_render_target(handle.id);
        if (opt_rtv.is_none()) {
            return foundation::Error(platform::RenderError::MakeError);
        }

        return opt_rtv.unwrap();
    }

    platform::RenderResult<std::shared_ptr<platform::IShaderResourceView>>
    D3D11Renderer::create_shader_resource_view(
        types::RenderHandle image_handle, const types::ImageViewDescription& description) {
        return foundation::Error(platform::RenderError::MakeError);
    }

    platform::RenderResult<std::shared_ptr<platform::IUnorderedAccessView>>
    D3D11Renderer::create_unordered_access_view(
        types::RenderHandle image_handle, const types::ImageViewDescription& description) {
        return foundation::Error(platform::RenderError::MakeError);
    }

    platform::RenderResult<types::RenderHandle> D3D11Renderer::create_mesh(
        types::MeshData&& mesh, const std::vector<types::RenderHandle>& shader_reflections) {
        const auto result = this->resource_manager->make_mesh(std::move(mesh), shader_reflections)
                                .add_message("メッシュの作成に失敗しました");
        if (result.is_err()) {
            return result.propagation(platform::RenderError::MakeError);
        }

        return result.unwrap();
    }

    platform::RenderResult<types::RenderHandle> D3D11Renderer::create_texture(
        const types::TextureData& texture) {
        const auto result = this->resource_manager->make_texture(texture).add_message(
            "テクスチャの作成に失敗しました");
        if (result.is_err()) {
            return result.propagation(platform::RenderError::MakeError);
        }

        return result.unwrap();
    }

    platform::RenderResult<types::RenderHandle> D3D11Renderer::create_shader(
        const types::ShaderKind kind, const types::ShaderData& shader_data) {
        const auto result = this->resource_manager->make_shader(kind, shader_data)
                                .add_message("シェーダーの作成に失敗しました");
        if (result.is_err()) {
            return result.propagation(platform::RenderError::MakeError);
        }

        return result.unwrap();
    }

    void D3D11Renderer::setup_viewports(void) const {
        // ビューポートのセット
        const auto& context = this->d3d11->get_context();
        const auto& viewports = this->resource_manager->get_native_resource_accessor()
                                    ->get_native_viewport_accessor()
                                    ->get_native_viewports();
        context->RSSetViewports(viewports.size(), viewports.data());
    }

    void D3D11Renderer::setup_render_targets(void) const {
        // レンダーターゲットのクリア
        const auto& context = this->d3d11->get_context();
        const auto& view_accessor =
            this->resource_manager->get_native_resource_accessor()->get_native_view_accessor();
        const auto& accessor = this->resource_manager->get_accessor();

        for (const auto& render_target : accessor->get_render_targets()) {
            const auto handle = render_target->get_handle();
            const auto opt_index = this->resource_manager->get_native_resource_index(handle);
            if (opt_index.is_none()) {
                continue;
            }
            const auto index = opt_index.unwrap();
            const auto opt_target = view_accessor->get_native_render_target_view(index.resource);
            if (opt_target.is_none()) {
                continue;
            }
            const auto& target = opt_target.unwrap();

            const auto color = render_target->get_clear_color();
            const float clear_color[4] = {color.r, color.g, color.b, color.a};
            context->ClearRenderTargetView(target.Get(), clear_color);
        }
    }

    void D3D11Renderer::bind_buffer(const types::RenderHandle& handle) const {
        const auto opt_index = this->resource_manager->get_native_resource_index(handle);
        if (opt_index.is_none()) {
            return;
        }
        const auto index = opt_index.unwrap();
        const auto opt_buffer = this->resource_manager->get_native_resource_accessor()
                                    ->get_native_buffer_accessor()
                                    ->get_native_buffer(index.resource);
        if (opt_buffer.is_none()) {
            return;
        }
        const auto opt_binding =
            this->resource_manager->get_resource_binder()->get_buffer_binding(index.binding);
        if (opt_binding.is_none()) {
            return;
        }

        const auto& buffer = opt_buffer.unwrap();
        const auto& binding = opt_binding.unwrap();
        const auto context = this->d3d11->get_context();
        if (const auto param = std::get_if<VertexBufferParameter>(&binding.parameter)) {
            context->IASetVertexBuffers(
                param->target, 1, buffer.GetAddressOf(), &param->stride, &param->offset);
        } else if (const auto param = std::get_if<IndexBufferParameter>(&binding.parameter)) {
            const auto format = [&param]() {
                switch (param->stride) {
                    case 1:
                        return DXGI_FORMAT::DXGI_FORMAT_R8_UINT;
                    case 2:
                        return DXGI_FORMAT::DXGI_FORMAT_R16_UINT;
                    case 4:
                        return DXGI_FORMAT::DXGI_FORMAT_R32_UINT;
                    default:
                        break;
                }
            }();
            context->IASetIndexBuffer(buffer.Get(), format, param->offset);
        } else if (const auto param = std::get_if<UniformBufferParameter>(&binding.parameter)) {
            switch (param->target_shader) {
                case types::ShaderKind::Vertex: {
                    context->VSSetConstantBuffers(param->target, 1, buffer.GetAddressOf());
                } break;
                case types::ShaderKind::Pixel: {
                    context->PSSetConstantBuffers(param->target, 1, buffer.GetAddressOf());
                } break;
                case types::ShaderKind::Compute: {
                    context->CSSetConstantBuffers(param->target, 1, buffer.GetAddressOf());
                } break;
                default:
                    break;
            }
        }
    }

    void D3D11Renderer::bind_shader(const types::RenderHandle& handle) const {
        const auto opt_index = this->resource_manager->get_native_resource_index(handle);
        if (opt_index.is_none()) {
            return;
        }
        const auto index = opt_index.unwrap();
        const auto shader_accessor =
            this->resource_manager->get_native_resource_accessor()->get_native_shader_accessor();
        const auto shader_type =
            shader_accessor->get_shader_kind(index.resource).value_or(types::ShaderKind::Unknown);
        const auto opt_binding =
            this->resource_manager->get_resource_binder()->get_buffer_binding(index.binding);
        if (opt_binding.is_none()) {
            return;
        }

        const auto context = this->d3d11->get_context();
        switch (shader_type) {
            case types::ShaderKind::Vertex: {
                const auto opt_shader = shader_accessor->get_native_vertex_shader(index.resource);
                if (opt_shader.is_none()) {
                    return;
                }
                context->VSSetShader(opt_shader.unwrap().Get(), nullptr, 0);
            } break;
            case types::ShaderKind::Pixel: {
                const auto opt_shader = shader_accessor->get_native_pixel_shader(index.resource);
                if (opt_shader.is_none()) {
                    return;
                }
                context->PSSetShader(opt_shader.unwrap().Get(), nullptr, 0);
            } break;
            case types::ShaderKind::Compute: {
                const auto opt_shader = shader_accessor->get_native_compute_shader(index.resource);
                if (opt_shader.is_none()) {
                    return;
                }
                context->CSSetShader(opt_shader.unwrap().Get(), nullptr, 0);
            } break;
            case types::ShaderKind::Hull: {
                const auto opt_shader = shader_accessor->get_native_hull_shader(index.resource);
                if (opt_shader.is_none()) {
                    return;
                }
                context->HSSetShader(opt_shader.unwrap().Get(), nullptr, 0);
            } break;
            default:
                break;
        }
    }

    void D3D11Renderer::bind_texture(const types::RenderHandle& handle) const {
        const auto opt_index = this->resource_manager->get_native_resource_index(handle);
        if (opt_index.is_none()) {
            return;
        }
        const auto index = opt_index.unwrap();
        const auto texture_accessor =
            this->resource_manager->get_native_resource_accessor()->get_native_texture_accessor();
        const auto opt_binding =
            this->resource_manager->get_resource_binder()->get_texture_binding(index.binding);
        if (opt_binding.is_none()) {
            return;
        }
        const auto& binding = opt_binding.unwrap();
        const auto context = this->d3d11->get_context();

        const auto opt_sampler = texture_accessor->get_native_sampler(index.resource);
        if (opt_sampler.is_none()) {
            return;
        }
        const auto& sampler = opt_sampler.unwrap();

        switch (binding.target_shader) {
            case types::ShaderKind::Vertex: {
                context->VSSetShaderResources(binding.target, 1, sampler.GetAddressOf());
                context->VSSetSamplers(binding.target, 1, sampler.GetAddressOf());
            } break;
            case types::ShaderKind::Pixel: {
                context->PSSetShaderResources(binding.target, 1, sampler.GetAddressOf());
                context->PSSetSamplers(binding.target, 1, sampler.GetAddressOf());
            } break;
            case types::ShaderKind::Compute: {
            } break;
            case types::ShaderKind::Hull: {
            } break;
            default:
                break;
        }

        /*
        const auto opt_texture = texture_accessor->get_native_texture_2d(index.resource);
        const auto opt_texture = texture_accessor->get_native_texture_3d(index.resource);
        */
    }

    void D3D11Renderer::bind_view(const types::RenderHandle& handle) const {
        const auto opt_index = this->resource_manager->get_native_resource_index(handle);
        if (opt_index.is_none()) {
            return;
        }
        const auto index = opt_index.unwrap();
        const auto view_accessor =
            this->resource_manager->get_native_resource_accessor()->get_native_view_accessor();
        const auto view_type =
            view_accessor->get_view_type(index.resource).value_or(types::ImageViewType::Unknown);

        const auto context = this->d3d11->get_context();
        switch (view_type) {
            case types::ImageViewType::DepthStencil: {
                const auto opt_view = view_accessor->get_native_depth_stencil_view(index.binding);
                if (opt_view.is_none()) {
                    return;
                }

                context->OMSetRenderTargets(1, nullptr, opt_view.unwrap().Get());
            } break;
            case types::ImageViewType::RenderTarget: {
                const auto opt_view = view_accessor->get_native_render_target_view(index.binding);
                if (opt_view.is_none()) {
                    return;
                }
                context->OMSetRenderTargets(1, opt_view.unwrap().GetAddressOf(), nullptr);
            } break;
            case types::ImageViewType::ShaderResource: {
                const auto opt_view = view_accessor->get_native_shader_resource_view(index.binding);
                if (opt_view.is_none()) {
                    return;
                }
            } break;
            case types::ImageViewType::UnorderedAccess: {
                const auto opt_view =
                    view_accessor->get_native_unordered_access_view(index.binding);
                if (opt_view.is_none()) {
                    return;
                }
            } break;
            default:
                break;
        }
    }

    void D3D11Renderer::bind_rasterizer(const types::RenderHandle& handle) const {
        const auto opt_index = this->resource_manager->get_native_resource_index(handle);
        if (opt_index.is_none()) {
            return;
        }
        const auto index = opt_index.unwrap();
        const auto opt_rasterizer = this->resource_manager->get_native_resource_accessor()
                                        ->get_native_rasterizer_accessor()
                                        ->get_native_rasterizer(index.resource);
        if (opt_rasterizer.is_none()) {
            return;
        }

        const auto context = this->d3d11->get_context();
        const auto& rasterizer = opt_rasterizer.unwrap();
        context->RSSetState(rasterizer.Get());
    }

    void D3D11Renderer::bind_mesh(const types::RenderHandle& handle) const {
        const auto& opt_mesh = this->resource_manager->get_mesh(handle.id);
        if (opt_mesh.is_none()) {
            return;
        }
        const auto& mesh = opt_mesh.unwrap();

        for (const auto handle : mesh.mesh_handles) {
            switch (handle.type) {
                case types::RenderHandleType::Buffer: {
                    this->bind_buffer(handle);
                } break;
                case types::RenderHandleType::Texture: {
                    this->bind_texture(handle);
                } break;
                case types::RenderHandleType::Draw: {
                    this->draw(handle);
                } break;
                default:
                    break;
            }
        }
    }

    void D3D11Renderer::bind_topology(const types::RenderHandle& handle) const {
        const auto topology = static_cast<types::PrimitiveTopology>(handle.id);
        const auto d3d11_topology = D3D11Converter::to_topology(topology);
        if (d3d11_topology == D3D11_PRIMITIVE_TOPOLOGY::D3D11_PRIMITIVE_TOPOLOGY_UNDEFINED) {
            return;
        }

        const auto context = this->d3d11->get_context();
        context->IASetPrimitiveTopology(d3d11_topology);
    }

    void D3D11Renderer::bind_input_layout(const types::RenderHandle& handle) const {
        const auto opt_index = this->resource_manager->get_native_resource_index(handle);
        if (opt_index.is_none()) {
            return;
        }
        const auto index = opt_index.unwrap();
        const auto opt_input_layout = this->resource_manager->get_native_resource_accessor()
                                          ->get_native_input_layout_accessor()
                                          ->get_native_input_layout(index.resource);
        if (opt_input_layout.is_none()) {
            return;
        }
        const auto& input_layout = opt_input_layout.unwrap();
        const auto context = this->d3d11->get_context();
        context->IASetInputLayout(input_layout.Get());
    }

    void D3D11Renderer::draw(const types::RenderHandle& handle) const {
        const auto opt_index = this->resource_manager->get_native_resource_index(handle);
        if (opt_index.is_none()) {
            return;
        }
        const auto index = opt_index.unwrap();
        const auto opt_buffer = this->resource_manager->get_native_resource_accessor()
                                    ->get_native_buffer_accessor()
                                    ->get_native_buffer(index.resource);
        if (opt_buffer.is_none()) {
            return;
        }
        const auto opt_binding =
            this->resource_manager->get_resource_binder()->get_draw_binding(index.binding);
        if (opt_binding.is_none()) {
            return;
        }

        const auto& binding = opt_binding.unwrap();
        const auto context = this->d3d11->get_context();
        if (const auto& argument = std::get_if<DrawParameter>(&binding.parameter)) {
            if (argument->instance_count > 1) {
                context->DrawInstanced(argument->vertex_count,
                    argument->instance_count,
                    argument->first_vertex,
                    argument->first_instance);
            } else {
                context->Draw(argument->vertex_count, argument->first_vertex);
            }
        } else if (const auto& argument = std::get_if<DrawIndexedParameter>(&binding.parameter)) {
            if (argument->instance_count > 1) {
                context->DrawIndexedInstanced(argument->index_count,
                    argument->instance_count,
                    argument->vertex_offset,
                    argument->first_index,
                    argument->first_instance);
            } else {
                context->DrawIndexed(
                    argument->index_count, argument->vertex_offset, argument->first_index);
            }
        }
    }

    void D3D11Renderer::present(void) const {
        this->d3d11->get_swap_chain()->Present(1, 0);
    }
} // namespace enishi::renderer::directx