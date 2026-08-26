#include "d3d11_renderer.h"
#include "d3d11_converter.h"
#include "resource/view/render_target_view.h"
#include <ranges>
#include <renderer/common/converter/model_to_mesh.h>

namespace enishi::renderer::directx {
    D3D11Renderer::D3D11Renderer(std::unique_ptr<D3D11> d3d11)
        : d3d11(std::move(d3d11))
        , updater_pool(std::make_unique<UpdaterPool>()) {
        this->resource_manager = std::make_unique<ResourceManager>(this->d3d11);
    }

    platform::IUpdaterAccessor* D3D11Renderer::get_updater_accessor(void) noexcept {
        return this->updater_pool.get();
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
        const types::RasterizerStateDescription& description) {
        const auto result = this->resource_manager->make_rasterizer_state(description)
                                .add_message("ラスタライザステートの作成に失敗しました");
        if (result.is_err()) {
            return result.propagation(platform::RenderError::MakeError);
        }

        return result.unwrap();
    }

    platform::RenderResult<types::RenderHandle> D3D11Renderer::create_sampler(
        const types::SamplerStateDescription& description) {
        const auto result = this->resource_manager->make_sampler_state(description)
                                .add_message("サンプラーステートの作成に失敗しました");
        if (result.is_err()) {
            return result.propagation(platform::RenderError::MakeError);
        }
        const auto& handle = result.unwrap();

        return result.unwrap();
    }

    platform::RenderResult<types::RenderHandle> D3D11Renderer::create_depth_stencil(
        const types::DepthStencilStateDescription& description) {
        const auto result = this->resource_manager->make_depth_stencil_state(description)
                                .add_message("サンプラーステートの作成に失敗しました");
        if (result.is_err()) {
            return result.propagation(platform::RenderError::MakeError);
        }
        const auto& handle = result.unwrap();

        return result.unwrap();
    }

    platform::RenderResult<types::RenderHandle> D3D11Renderer::create_blend(
        const types::BlendStateDescription& description) {
        const auto result = this->resource_manager->make_blend_state(description)
                                .add_message("ブレンドステートの作成に失敗しました");
        if (result.is_err()) {
            return result.propagation(platform::RenderError::MakeError);
        }
        const auto& handle = result.unwrap();

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
        const auto result = this->resource_manager->make_view(image_handle, description)
                                .add_message("ビューの作成に失敗しました");
        if (result.is_err()) {
            return result.propagation(platform::RenderError::MakeError);
        }
        const auto& handle = result.unwrap();
        const auto opt_index = this->resource_manager->get_native_resource_handle(handle);
        if (opt_index.is_none()) {
            return foundation::Error(platform::RenderError::MakeError);
        }
        const auto& index = opt_index.unwrap();
        auto opt_rtv = this->resource_manager->get_resource_accessor()
                           ->get_view_accessor()
                           ->get_render_target_view(index.configurable);
        if (opt_rtv.is_none()) {
            return foundation::Error(
                platform::RenderError::MakeError, "レンダーターゲットが見つかりません");
        }

        return opt_rtv.unwrap();
    }

    platform::RenderResult<std::shared_ptr<platform::IDepthStencilView>>
    D3D11Renderer::create_depth_stencil_view(
        types::RenderHandle image_handle, const types::ImageViewDescription& description) {
        const auto result = this->resource_manager->make_view(image_handle, description)
                                .add_message("ビューの作成に失敗しました");
        if (result.is_err()) {
            return result.propagation(platform::RenderError::MakeError);
        }
        const auto& handle = result.unwrap();
        const auto opt_index = this->resource_manager->get_native_resource_handle(handle);
        if (opt_index.is_none()) {
            return foundation::Error(platform::RenderError::MakeError);
        }
        const auto& index = opt_index.unwrap();
        auto opt_rtv = this->resource_manager->get_resource_accessor()
                           ->get_view_accessor()
                           ->get_depth_stencil_view(index.configurable);
        if (opt_rtv.is_none()) {
            return foundation::Error(platform::RenderError::MakeError);
        }

        return opt_rtv.unwrap();
    }

    platform::RenderResult<std::shared_ptr<platform::IShaderResourceView>>
    D3D11Renderer::create_shader_resource_view(
        types::RenderHandle image_handle, const types::ImageViewDescription& description) {
        const auto result = this->resource_manager->make_view(image_handle, description)
                                .add_message("ビューの作成に失敗しました");
        if (result.is_err()) {
            return result.propagation(platform::RenderError::MakeError);
        }
        const auto& handle = result.unwrap();
        const auto opt_index = this->resource_manager->get_native_resource_handle(handle);
        if (opt_index.is_none()) {
            return foundation::Error(platform::RenderError::MakeError);
        }
        const auto& index = opt_index.unwrap();
        auto opt_rtv = this->resource_manager->get_resource_accessor()
                           ->get_view_accessor()
                           ->get_shader_resource_view(index.configurable);
        if (opt_rtv.is_none()) {
            return foundation::Error(platform::RenderError::MakeError);
        }

        return opt_rtv.unwrap();
    }

    platform::RenderResult<std::shared_ptr<platform::IUnorderedAccessView>>
    D3D11Renderer::create_unordered_access_view(
        types::RenderHandle image_handle, const types::ImageViewDescription& description) {
        const auto result = this->resource_manager->make_view(image_handle, description)
                                .add_message("ビューの作成に失敗しました");
        if (result.is_err()) {
            return result.propagation(platform::RenderError::MakeError);
        }
        const auto& handle = result.unwrap();
        const auto opt_index = this->resource_manager->get_native_resource_handle(handle);
        if (opt_index.is_none()) {
            return foundation::Error(platform::RenderError::MakeError);
        }
        const auto& index = opt_index.unwrap();
        auto opt_rtv = this->resource_manager->get_resource_accessor()
                           ->get_view_accessor()
                           ->get_unodered_access_view(index.configurable);
        if (opt_rtv.is_none()) {
            return foundation::Error(platform::RenderError::MakeError);
        }

        return opt_rtv.unwrap();
    }

    platform::RenderResult<types::RenderHandle> D3D11Renderer::create_mesh(
        const types::ModelData& model_data,
        const std::vector<types::RenderHandle>& shader_reflections) {
        auto&& mesh_result = ModelToMesh::to_mesh_data(model_data)
                                 .add_message("メッシュデータへの変換に失敗しました");
        if (mesh_result.is_err()) {
            return mesh_result.propagation(platform::RenderError::MakeError);
        }
        const auto result = this->resource_manager
                                ->make_mesh(std::move(mesh_result).unwrap_mut(), shader_reflections)
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

    void D3D11Renderer::setup_views(void) const {
        // レンダーターゲットのクリア
        const auto& context = this->d3d11->get_context();
        const auto& view_accessor =
            this->resource_manager->get_native_resource_accessor()->get_native_view_accessor();
        const auto& accessor = this->resource_manager->get_resource_accessor()->get_view_accessor();

        for (const auto& render_target : accessor->get_render_target_views()) {
            const auto handle = render_target->get_handle();
            const auto opt_index = this->resource_manager->get_native_resource_handle(handle);
            if (opt_index.is_none()) {
                continue;
            }
            const auto& index = opt_index.unwrap();
            const auto opt_target = view_accessor->get_native_render_target_view(index.resource);
            if (opt_target.is_none()) {
                continue;
            }
            const auto& target = opt_target.unwrap();

            const auto color = render_target->get_clear_color();
            const float clear_color[4] = {color.r, color.g, color.b, color.a};
            context->ClearRenderTargetView(target.Get(), clear_color);
        }

        // 深度情報のクリア
        for (const auto& depth_stnencil : accessor->get_depth_stencil_views()) {
            const auto handle = depth_stnencil->get_handle();
            const auto opt_index = this->resource_manager->get_native_resource_handle(handle);
            if (opt_index.is_none()) {
                continue;
            }
            const auto& index = opt_index.unwrap();
            const auto opt_depth_stencil_view =
                view_accessor->get_native_depth_stencil_view(index.resource);
            if (opt_depth_stencil_view.is_none()) {
                continue;
            }
            const auto& native_depth_stencil_view = opt_depth_stencil_view.unwrap();

            context->ClearDepthStencilView(native_depth_stencil_view.Get(),
                D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL,
                depth_stnencil->clear_depth(),
                depth_stnencil->clear_stencil());
        }
    }

    void D3D11Renderer::submit_command_buffer(const types::DrawCommand& command) const {
        switch (command.sub_command) {
            case types::SubCommand::Bind: {
                this->bind_buffer(command.handle);
            } break;
            case types::SubCommand::Unbind: {
            } break;
            default:
                break;
        }
    }

    void D3D11Renderer::submit_command_shader(const types::DrawCommand& command) const {
        switch (command.sub_command) {
            case types::SubCommand::Bind: {
                this->bind_shader(command.handle);
            } break;
            case types::SubCommand::Unbind: {
            } break;
            default:
                break;
        }
    }

    void D3D11Renderer::submit_command_view(
        const types::DrawCommand& command, const types::RenderHandle& handle) const {
        switch (command.sub_command) {
            case types::SubCommand::Bind: {
                this->bind_view(command.handle, handle);
            } break;
            case types::SubCommand::Unbind: {
            } break;
            default:
                break;
        }
    }

    void D3D11Renderer::submit_command_mesh(const types::DrawCommand& command) const {
        switch (command.sub_command) {
            case types::SubCommand::Bind: {
                this->bind_mesh(command.handle);
            } break;
            case types::SubCommand::Unbind: {
            } break;
            default:
                break;
        }
    }

    void D3D11Renderer::submit_command_topology(const types::DrawCommand& command) const {
        switch (command.sub_command) {
            case types::SubCommand::Bind: {
                this->bind_topology(command.handle);
            } break;
            case types::SubCommand::Unbind: {
            } break;
            default:
                break;
        }
    }

    void D3D11Renderer::submit_command_vertex_layout(const types::DrawCommand& command) const {
        switch (command.sub_command) {
            case types::SubCommand::Bind: {
                this->bind_input_layout(command.handle);
            } break;
            case types::SubCommand::Unbind: {
            } break;
            default:
                break;
        }
    }

    void D3D11Renderer::submit_command_viewport(const types::DrawCommand& command) const {
        switch (command.sub_command) {
            case types::SubCommand::Bind: {
            } break;
            case types::SubCommand::Unbind: {
            } break;
            default:
                break;
        }
    }

    void D3D11Renderer::submit_command_state(const types::DrawCommand& command) const {
        switch (command.sub_command) {
            case types::SubCommand::Bind: {
                this->bind_state(command.handle);
            } break;
            case types::SubCommand::Unbind: {
            } break;
            default:
                break;
        }
    }

    void D3D11Renderer::submit_command_image(const types::DrawCommand& command) const {
        switch (command.sub_command) {
            case types::SubCommand::Bind: {
                this->bind_image(command.handle);
            } break;
            case types::SubCommand::Unbind: {
            } break;
            default:
                break;
        }
    }

    void D3D11Renderer::draw(const types::RenderHandle& handle) const {
        const auto opt_index = this->resource_manager->get_native_resource_handle(handle);
        if (opt_index.is_none()) {
            return;
        }
        const auto& index = opt_index.unwrap();
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

    void D3D11Renderer::bind_buffer(const types::RenderHandle& handle) const {
        const auto opt_index = this->resource_manager->get_native_resource_handle(handle);
        if (opt_index.is_none()) {
            return;
        }
        const auto& index = opt_index.unwrap();
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
        const auto opt_index = this->resource_manager->get_native_resource_handle(handle);
        if (opt_index.is_none()) {
            return;
        }
        const auto& index = opt_index.unwrap();
        const auto shader_accessor =
            this->resource_manager->get_native_resource_accessor()->get_native_shader_accessor();
        const auto shader_type =
            shader_accessor->get_shader_kind(index.resource).unwrap_or(types::ShaderKind::Unknown);
        const auto opt_binding =
            this->resource_manager->get_resource_binder()->get_shader_binding(index.binding);
        if (opt_binding.is_none()) {
            return;
        }

        const auto context = this->d3d11->get_context();
        const auto& binding = opt_binding.unwrap();
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

    void D3D11Renderer::bind_view(const types::RenderHandle& bind_handle,
        const types::RenderHandle& render_target_handle) const {
        const auto opt_index = this->resource_manager->get_native_resource_handle(bind_handle);
        if (opt_index.is_none()) {
            return;
        }
        const auto& index = opt_index.unwrap();
        const auto view_type = this->resource_manager->get_resource_accessor()
                                   ->get_view_accessor()
                                   ->get_view_type(index.resource)
                                   .unwrap_or(types::ImageViewType::Unknown);
        const auto opt_binding =
            this->resource_manager->get_resource_binder()->get_view_binding(index.binding);
        if (opt_binding.is_none()) {
            return;
        }

        const auto view_accessor =
            this->resource_manager->get_native_resource_accessor()->get_native_view_accessor();
        const auto& binding = opt_binding.unwrap();
        const auto context = this->d3d11->get_context();
        switch (view_type) {
            case types::ImageViewType::DepthStencil: {
                const auto opt_rtv_index =
                    this->resource_manager->get_native_resource_handle(render_target_handle);
                if (opt_rtv_index.is_none()) {
                    return;
                }
                const auto opt_dsv = view_accessor->get_native_depth_stencil_view(index.resource);
                if (opt_dsv.is_none()) {
                    return;
                }
                const auto opt_bind_param = binding.get_depth_stencil_param();
                if (opt_bind_param.is_none()) {
                    return;
                }
                const auto& dsv = opt_dsv.unwrap();

                const auto& rtv_index = opt_rtv_index.unwrap();
                const auto opt_rtv =
                    view_accessor->get_native_render_target_view(rtv_index.resource);
                if (opt_rtv.is_some()) {
                    const auto& rtv = opt_rtv.unwrap();
                    context->OMSetRenderTargets(1, rtv.GetAddressOf(), dsv.Get());
                } else {
                    context->OMSetRenderTargets(0, nullptr, dsv.Get());
                }
            } break;
            case types::ImageViewType::RenderTarget: {
                const auto opt_view = view_accessor->get_native_render_target_view(index.resource);
                if (opt_view.is_none()) {
                    return;
                }
                const auto opt_bind_param = binding.get_render_target_param();
                if (opt_bind_param.is_none()) {
                    return;
                }
                const auto& view = opt_view.unwrap();

                context->OMSetRenderTargets(1, view.GetAddressOf(), nullptr);
            } break;
            case types::ImageViewType::ShaderResource: {
                const auto opt_view =
                    view_accessor->get_native_shader_resource_view(index.resource);
                if (opt_view.is_none()) {
                    return;
                }
                const auto opt_bind_param = binding.get_shader_resource_param();
                if (opt_bind_param.is_none()) {
                    return;
                }
                const auto bind_param = opt_bind_param.unwrap();

                const auto& view = opt_view.unwrap();
                switch (bind_param.target_shader) {
                    case types::ShaderKind::Vertex: {
                        context->VSSetShaderResources(bind_param.target, 1, view.GetAddressOf());
                    } break;
                    case types::ShaderKind::Pixel: {
                        context->PSSetShaderResources(bind_param.target, 1, view.GetAddressOf());
                    } break;
                    case types::ShaderKind::Compute: {
                        context->CSSetShaderResources(bind_param.target, 1, view.GetAddressOf());
                    } break;
                    case types::ShaderKind::Hull: {
                        context->HSSetShaderResources(bind_param.target, 1, view.GetAddressOf());
                    } break;
                    default:
                        break;
                }
            } break;
            case types::ImageViewType::UnorderedAccess: {
                const auto opt_view =
                    view_accessor->get_native_unordered_access_view(index.resource);
                if (opt_view.is_none()) {
                    return;
                }
                const auto opt_bind_param = binding.get_unodered_access_param();
                if (opt_bind_param.is_none()) {
                    return;
                }
                const auto& view = opt_view.unwrap();
            } break;
            default:
                break;
        }
    }

    void D3D11Renderer::bind_state(const types::RenderHandle& handle) const {
        const auto opt_index = this->resource_manager->get_native_resource_handle(handle);
        if (opt_index.is_none()) {
            return;
        }
        const auto& index = opt_index.unwrap();
        const auto state_kind = this->resource_manager->get_resource_accessor()
                                    ->get_state_accessor()
                                    ->get_state_kind(index.resource)
                                    .unwrap_or(types::StateKind::Unknown);
        const auto opt_state_binding =
            this->resource_manager->get_resource_binder()->get_state_binding(index.binding);
        if (opt_state_binding.is_none()) {
            return;
        }

        const auto state_accessor =
            this->resource_manager->get_native_resource_accessor()->get_native_state_accessor();
        const auto& state_binding = opt_state_binding.unwrap();
        const auto context = this->d3d11->get_context();
        switch (state_kind) {
            case types::StateKind::Rasterizer: {
                const auto opt_state = state_accessor->get_native_rasterizer_state(index.resource);
                if (opt_state.is_none()) {
                    return;
                }
                const auto& state = opt_state.unwrap();
                context->RSSetState(state.Get());
            } break;
            case types::StateKind::Blend: {
                const auto opt_state = state_accessor->get_native_blend_state(index.resource);
                if (opt_state.is_none()) {
                    return;
                }
                const auto& state = opt_state.unwrap();

                // TODO: インターフェイス経由で変更可能にする
                constexpr float BLEND_COLOR[4] = {0.0f, 0.0f, 0.0f, 0.0f};
                context->OMSetBlendState(state.Get(), BLEND_COLOR, 0xFFFFFFFF);
            } break;
            case types::StateKind::DepthStencil: {
                const auto opt_state =
                    state_accessor->get_native_depth_stencil_state(index.resource);
                if (opt_state.is_none()) {
                    return;
                }
                const auto& state = opt_state.unwrap();
                context->OMSetDepthStencilState(state.Get(), 0);
            } break;
            case types::StateKind::Sampler: {
                const auto opt_state = state_accessor->get_native_sampler_state(index.resource);
                if (opt_state.is_none()) {
                    return;
                }
                const auto opt_binding = state_binding.get_sampler_state_param();
                if (opt_binding.is_none()) {
                    return;
                }
                const auto& binding = opt_binding.unwrap();

                const auto& state = opt_state.unwrap();
                switch (binding.target_shader) {
                    case types::ShaderKind::Vertex: {
                        context->VSSetSamplers(binding.target, 1, state.GetAddressOf());
                    } break;
                    case types::ShaderKind::Pixel: {
                        context->PSSetSamplers(binding.target, 1, state.GetAddressOf());
                    } break;
                    case types::ShaderKind::Compute: {
                        context->CSSetSamplers(binding.target, 1, state.GetAddressOf());
                    } break;
                    case types::ShaderKind::Hull: {
                        context->HSSetSamplers(binding.target, 1, state.GetAddressOf());
                    } break;
                    default:
                        break;
                }
            } break;

            default:
                break;
        }
    }

    void D3D11Renderer::bind_image(const types::RenderHandle& handle) const {
        const auto opt_index = this->resource_manager->get_native_resource_handle(handle);
        if (opt_index.is_none()) {
            return;
        }
        const auto& index = opt_index.unwrap();
    }

    void D3D11Renderer::bind_mesh(const types::RenderHandle& handle) const {
        const auto opt_index = this->resource_manager->get_native_resource_handle(handle);
        if (opt_index.is_none()) {
            return;
        }
        const auto& index = opt_index.unwrap();
        const auto& opt_mesh =
            this->resource_manager->get_resource_accessor()->get_mesh_accessor()->get_mesh_handles(
                index.resource);
        if (opt_mesh.is_none()) {
            return;
        }
        const auto& mesh = opt_mesh.unwrap();

        for (const auto& handle : mesh.mesh_handles) {
            switch (handle.type) {
                case types::RenderHandleType::Buffer: {
                    this->bind_buffer(handle);
                } break;
                case types::RenderHandleType::Shader: {
                    this->bind_shader(handle);
                } break;
                case types::RenderHandleType::State: {
                    this->bind_state(handle);
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
        const auto topology = static_cast<types::PrimitiveTopology>(handle.id.handle_id);
        const auto d3d11_topology = D3D11Converter::to_topology(topology);
        if (d3d11_topology == D3D11_PRIMITIVE_TOPOLOGY::D3D11_PRIMITIVE_TOPOLOGY_UNDEFINED) {
            return;
        }

        const auto context = this->d3d11->get_context();
        context->IASetPrimitiveTopology(d3d11_topology);
    }

    void D3D11Renderer::bind_input_layout(const types::RenderHandle& handle) const {
        const auto opt_index = this->resource_manager->get_native_resource_handle(handle);
        if (opt_index.is_none()) {
            return;
        }
        const auto& index = opt_index.unwrap();
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
} // namespace enishi::renderer::directx