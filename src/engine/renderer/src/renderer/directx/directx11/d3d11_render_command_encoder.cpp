#include "d3d11_render_command_encoder.h"
#include <ranges>
#include <renderer/directx/directx11/d3d11_converter.h>
#include <renderer/directx/directx11/resource/view/render_target_view.h>
#include <utility>

namespace enishi::renderer::directx {
    namespace {
        void set_attachment_viewport(ID3D11DeviceContext* context, ID3D11View* view) {
            Microsoft::WRL::ComPtr<ID3D11Resource> resource;
            view->GetResource(resource.GetAddressOf());
            Microsoft::WRL::ComPtr<ID3D11Texture2D> texture;
            if (FAILED(resource.As(&texture))) {
                return;
            }
            D3D11_TEXTURE2D_DESC description{};
            texture->GetDesc(&description);
            const D3D11_VIEWPORT viewport{0,
                0,
                static_cast<float>(description.Width),
                static_cast<float>(description.Height),
                0,
                1};
            context->RSSetViewports(1, &viewport);
        }
    } // namespace
    D3D11RenderCommandEncoder::D3D11RenderCommandEncoder(
        std::shared_ptr<ID3D11RenderResources> resources)
        : resources(std::move(resources)) {
    }
    void D3D11RenderCommandEncoder::setup_viewports(void) const {
        // ビューポートのセット
        const auto& context = this->resources->get_context()->get_context();
        const auto& viewports = this->resources->get_resource_manager()
                                    .get_native_resource_accessor()
                                    ->get_native_viewport_accessor()
                                    ->get_native_viewports();
        context->RSSetViewports(viewports.size(), viewports.data());
    }
    void D3D11RenderCommandEncoder::setup_views(void) const {
        // The previous frame sampled the shadow depth; detach SRVs before clearing/writing it.
        ID3D11ShaderResourceView* empty_views[D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT]{};
        this->resources->get_context()->get_context()->PSSetShaderResources(
            0, D3D11_COMMONSHADER_INPUT_RESOURCE_SLOT_COUNT, empty_views);

        // レンダーターゲットのクリア
        const auto& context = this->resources->get_context()->get_context();
        const auto& view_accessor = this->resources->get_resource_manager()
                                        .get_native_resource_accessor()
                                        ->get_native_view_accessor();
        const auto& accessor =
            this->resources->get_resource_manager().get_resource_accessor()->get_view_accessor();

        for (const auto& render_target : accessor->get_render_target_views()) {
            const auto handle = render_target->get_handle();
            const auto opt_index =
                this->resources->get_resource_manager().get_render_handle_mapper()->get(handle);
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
            const auto opt_index =
                this->resources->get_resource_manager().get_render_handle_mapper()->get(handle);
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
                depth_stnencil->get_description().format == types::ImageFormat::D24_UNORM_S8_UINT
                    ? D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL
                    : D3D11_CLEAR_DEPTH,
                depth_stnencil->clear_depth(),
                depth_stnencil->clear_stencil());
        }
    }
    void D3D11RenderCommandEncoder::submit_command_buffer(const types::DrawCommand& command) const {
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
    void D3D11RenderCommandEncoder::submit_command_shader(const types::DrawCommand& command) const {
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
    void D3D11RenderCommandEncoder::submit_command_view(
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
    void D3D11RenderCommandEncoder::submit_command_mesh(
        const types::DrawCommand& command, const types::RenderHandle& handle) const {
        switch (command.sub_command) {
            case types::SubCommand::Bind: {
                this->bind_mesh(command.handle, handle);
            } break;
            case types::SubCommand::Unbind: {
            } break;
            default:
                break;
        }
    }
    void D3D11RenderCommandEncoder::submit_command_topology(
        const types::DrawCommand& command) const {
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
    void D3D11RenderCommandEncoder::submit_command_vertex_layout(
        const types::DrawCommand& command) const {
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
    void D3D11RenderCommandEncoder::submit_command_viewport(
        const types::DrawCommand& command) const {
        switch (command.sub_command) {
            case types::SubCommand::Bind: {
            } break;
            case types::SubCommand::Unbind: {
            } break;
            default:
                break;
        }
    }
    void D3D11RenderCommandEncoder::submit_command_state(const types::DrawCommand& command) const {
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
    void D3D11RenderCommandEncoder::submit_command_image(const types::DrawCommand& command) const {
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
    void D3D11RenderCommandEncoder::draw(const types::RenderHandle& handle) const {
        const auto opt_index =
            this->resources->get_resource_manager().get_render_handle_mapper()->get(handle);
        if (opt_index.is_none()) {
            return;
        }
        const auto& index = opt_index.unwrap();
        const auto opt_buffer = this->resources->get_resource_manager()
                                    .get_native_resource_accessor()
                                    ->get_native_buffer_accessor()
                                    ->get_native_buffer(index.resource);
        if (opt_buffer.is_none()) {
            return;
        }
        const auto opt_binding =
            this->resources->get_resource_manager().get_resource_binder()->get_draw_binding(
                index.binding);
        if (opt_binding.is_none()) {
            return;
        }

        const auto& binding = opt_binding.unwrap();
        const auto context = this->resources->get_context()->get_context();
        const auto query = this->resources->get_context()->get_query();

        context->Begin(query.Get());

        if (const auto& argument = std::get_if<types::DrawParameter>(&binding.parameter)) {
            if (argument->instance_count > 1) {
                context->DrawInstanced(argument->vertex_count,
                    argument->instance_count,
                    argument->first_vertex,
                    argument->first_instance);
            } else {
                context->Draw(argument->vertex_count, argument->first_vertex);
            }
        } else if (const auto& argument =
                       std::get_if<types::DrawIndexedParameter>(&binding.parameter)) {
            if (argument->instance_count > 1) {
                context->DrawIndexedInstanced(argument->index_count,
                    argument->instance_count,
                    argument->first_index,
                    argument->vertex_offset,
                    argument->first_instance);
            } else {
                context->DrawIndexed(
                    argument->index_count, argument->first_index, argument->vertex_offset);
            }
        }

        context->End(query.Get());
        D3D11_QUERY_DATA_PIPELINE_STATISTICS stats{};
        UINT64 passedSamples = 0;
        HRESULT hr = S_FALSE;
        for (; hr == S_FALSE;) {
            hr = context->GetData(query.Get(), &stats, sizeof(stats), 0);
            if (FAILED(hr)) {
                break;
            }

            if (hr == S_FALSE) {
                ::Sleep(0);
            }
        }

        if (hr == S_OK) {
            stats.IAVertices;
            stats.IAPrimitives;
            stats.VSInvocations;
            stats.CInvocations;
            stats.CPrimitives;
            stats.PSInvocations;
        }
        /*
         */
    }
    void D3D11RenderCommandEncoder::present(void) const {
        this->resources->get_context()->get_swap_chain()->Present(1, 0);
    }
    void D3D11RenderCommandEncoder::bind_buffer(const types::RenderHandle& handle) const {
        const auto opt_index =
            this->resources->get_resource_manager().get_render_handle_mapper()->get(handle);
        if (opt_index.is_none()) {
            return;
        }
        const auto& index = opt_index.unwrap();
        const auto opt_buffer = this->resources->get_resource_manager()
                                    .get_native_resource_accessor()
                                    ->get_native_buffer_accessor()
                                    ->get_native_buffer(index.resource);
        if (opt_buffer.is_none()) {
            return;
        }
        const auto opt_binding =
            this->resources->get_resource_manager().get_resource_binder()->get_buffer_binding(
                index.binding);
        if (opt_binding.is_none()) {
            return;
        }

        const auto& buffer = opt_buffer.unwrap();
        const auto& binding = opt_binding.unwrap();
        const auto context = this->resources->get_context()->get_context();
        if (const auto param = std::get_if<types::VertexBufferParameter>(&binding.parameter)) {
            context->IASetVertexBuffers(
                param->target, 1, buffer.GetAddressOf(), &param->stride, &param->offset);
        } else if (const auto param =
                       std::get_if<types::IndexBufferParameter>(&binding.parameter)) {
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
        } else if (const auto param =
                       std::get_if<types::UniformBufferParameter>(&binding.parameter)) {
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
    void D3D11RenderCommandEncoder::bind_shader(const types::RenderHandle& handle) const {
        const auto opt_index =
            this->resources->get_resource_manager().get_render_handle_mapper()->get(handle);
        if (opt_index.is_none()) {
            return;
        }
        const auto& index = opt_index.unwrap();
        const auto shader_accessor = this->resources->get_resource_manager()
                                         .get_native_resource_accessor()
                                         ->get_native_shader_accessor();
        const auto shader_type =
            shader_accessor->get_shader_kind(index.resource).unwrap_or(types::ShaderKind::Unknown);
        const auto opt_binding =
            this->resources->get_resource_manager().get_resource_binder()->get_shader_binding(
                index.binding);
        if (opt_binding.is_none()) {
            return;
        }

        const auto context = this->resources->get_context()->get_context();
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
    void D3D11RenderCommandEncoder::bind_view(const types::RenderHandle& bind_handle,
        const types::RenderHandle& render_target_handle) const {
        const auto opt_index =
            this->resources->get_resource_manager().get_render_handle_mapper()->get(bind_handle);
        if (opt_index.is_none()) {
            return;
        }
        const auto& index = opt_index.unwrap();
        const auto view_type = this->resources->get_resource_manager()
                                   .get_resource_accessor()
                                   ->get_view_accessor()
                                   ->get_view_type(index.resource)
                                   .unwrap_or(types::ImageViewType::Unknown);
        const auto opt_binding =
            this->resources->get_resource_manager().get_resource_binder()->get_view_binding(
                index.binding);
        if (opt_binding.is_none()) {
            return;
        }

        const auto view_accessor = this->resources->get_resource_manager()
                                       .get_native_resource_accessor()
                                       ->get_native_view_accessor();
        const auto& binding = opt_binding.unwrap();
        const auto context = this->resources->get_context()->get_context();
        switch (view_type) {
            case types::ImageViewType::DepthStencil: {
                const auto opt_rtv_index =
                    this->resources->get_resource_manager().get_render_handle_mapper()->get(
                        render_target_handle);
                const auto opt_dsv = view_accessor->get_native_depth_stencil_view(index.resource);
                if (opt_dsv.is_none()) {
                    return;
                }
                auto param = std::get_if<types::DepthStencilParameter>(&binding.parameter);
                if (!bool(param)) {
                    return;
                }
                const auto& dsv = opt_dsv.unwrap();

                set_attachment_viewport(context.Get(), dsv.Get());
                if (opt_rtv_index.is_none()) {
                    context->OMSetRenderTargets(0, nullptr, dsv.Get());
                    break;
                }
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
                auto param = std::get_if<types::RenderTargetParameter>(&binding.parameter);
                if (!bool(param)) {
                    return;
                }
                const auto& view = opt_view.unwrap();

                context->OMSetRenderTargets(1, view.GetAddressOf(), nullptr);
                set_attachment_viewport(context.Get(), view.Get());
            } break;
            case types::ImageViewType::ShaderResource: {
                const auto opt_view =
                    view_accessor->get_native_shader_resource_view(index.resource);
                if (opt_view.is_none()) {
                    return;
                }
                auto param = std::get_if<types::ShaderResourceParameter>(&binding.parameter);
                if (!bool(param)) {
                    return;
                }

                const auto& view = opt_view.unwrap();
                switch (param->target_shader) {
                    case types::ShaderKind::Vertex: {
                        context->VSSetShaderResources(param->target, 1, view.GetAddressOf());
                    } break;
                    case types::ShaderKind::Pixel: {
                        context->PSSetShaderResources(param->target, 1, view.GetAddressOf());
                    } break;
                    case types::ShaderKind::Compute: {
                        context->CSSetShaderResources(param->target, 1, view.GetAddressOf());
                    } break;
                    case types::ShaderKind::Hull: {
                        context->HSSetShaderResources(param->target, 1, view.GetAddressOf());
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
                auto param = std::get_if<types::UnorderedAccessParameter>(&binding.parameter);
                if (!bool(param)) {
                    return;
                }
                const auto& view = opt_view.unwrap();
            } break;
            default:
                break;
        }
    }
    void D3D11RenderCommandEncoder::bind_state(const types::RenderHandle& handle) const {
        const auto opt_index =
            this->resources->get_resource_manager().get_render_handle_mapper()->get(handle);
        if (opt_index.is_none()) {
            return;
        }
        const auto& index = opt_index.unwrap();
        const auto state_kind = this->resources->get_resource_manager()
                                    .get_resource_accessor()
                                    ->get_state_accessor()
                                    ->get_state_kind(index.resource)
                                    .unwrap_or(types::StateKind::Unknown);
        const auto opt_state_binding =
            this->resources->get_resource_manager().get_resource_binder()->get_state_binding(
                index.binding);
        if (opt_state_binding.is_none()) {
            return;
        }

        const auto state_accessor = this->resources->get_resource_manager()
                                        .get_native_resource_accessor()
                                        ->get_native_state_accessor();
        const auto& state_binding = opt_state_binding.unwrap();
        const auto context = this->resources->get_context()->get_context();
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
                auto param = std::get_if<types::SamplerStateParameter>(&state_binding.parameter);
                if (!bool(param)) {
                    return;
                }

                const auto& state = opt_state.unwrap();
                switch (param->target_shader) {
                    case types::ShaderKind::Vertex: {
                        context->VSSetSamplers(param->target, 1, state.GetAddressOf());
                    } break;
                    case types::ShaderKind::Pixel: {
                        context->PSSetSamplers(param->target, 1, state.GetAddressOf());
                    } break;
                    case types::ShaderKind::Compute: {
                        context->CSSetSamplers(param->target, 1, state.GetAddressOf());
                    } break;
                    case types::ShaderKind::Hull: {
                        context->HSSetSamplers(param->target, 1, state.GetAddressOf());
                    } break;
                    default:
                        break;
                }
            } break;

            default:
                break;
        }
    }
    void D3D11RenderCommandEncoder::bind_image(const types::RenderHandle& handle) const {
        const auto opt_index =
            this->resources->get_resource_manager().get_render_handle_mapper()->get(handle);
        if (opt_index.is_none()) {
            return;
        }
        const auto& index = opt_index.unwrap();
    }
    void D3D11RenderCommandEncoder::bind_mesh(
        const types::RenderHandle& handle, const types::RenderHandle& render_target_handle) const {
        const auto opt_index =
            this->resources->get_resource_manager().get_render_handle_mapper()->get(handle);
        if (opt_index.is_none()) {
            return;
        }
        const auto& index = opt_index.unwrap();
        const auto& opt_mesh = this->resources->get_resource_manager()
                                   .get_resource_accessor()
                                   ->get_mesh_accessor()
                                   ->get_mesh_handle(index.resource);
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
                case types::RenderHandleType::View: {
                    this->bind_view(handle, render_target_handle);
                } break;
                case types::RenderHandleType::Draw: {
                    this->draw(handle);
                } break;
                default:
                    break;
            }
        }
    }
    void D3D11RenderCommandEncoder::bind_topology(const types::RenderHandle& handle) const {
        const auto topology = static_cast<types::PrimitiveTopology>(handle.id.handle_id);
        const auto d3d11_topology = D3D11Converter::to_topology(topology);
        if (d3d11_topology == D3D11_PRIMITIVE_TOPOLOGY::D3D11_PRIMITIVE_TOPOLOGY_UNDEFINED) {
            return;
        }

        const auto context = this->resources->get_context()->get_context();
        context->IASetPrimitiveTopology(d3d11_topology);
    }
    void D3D11RenderCommandEncoder::bind_input_layout(const types::RenderHandle& handle) const {
        const auto opt_index =
            this->resources->get_resource_manager().get_render_handle_mapper()->get(handle);
        if (opt_index.is_none()) {
            return;
        }
        const auto& index = opt_index.unwrap();
        const auto opt_input_layout = this->resources->get_resource_manager()
                                          .get_native_resource_accessor()
                                          ->get_native_input_layout_accessor()
                                          ->get_native_input_layout(index.resource);
        if (opt_input_layout.is_none()) {
            return;
        }
        const auto& input_layout = opt_input_layout.unwrap();
        const auto context = this->resources->get_context()->get_context();
        context->IASetInputLayout(input_layout.Get());
    }
} // namespace enishi::renderer::directx
