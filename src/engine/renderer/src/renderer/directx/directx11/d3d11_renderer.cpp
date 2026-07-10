#include "d3d11_renderer.h"
#include "d3d11_converter.h"
#include "shader/shader_refrection.h"
#include "view/render_target_view.h"
#include <ranges>

namespace enishi::renderer::directx {
    void D3D11Renderer::execute(
        ID3D11DeviceContext* const context, const types::DrawCommand& command) const {
        this->bind_handle(context, command.handle);
    }

    void D3D11Renderer::bind_handle(
        ID3D11DeviceContext* const context, const types::RenderHandle handle) const {
        const auto id = handle.id;
        switch (handle.type) {
            case types::RenderHandleType::Buffer: {
                const auto& opt_buffer = this->resource_manager.get_buffer(id);
                if (!opt_buffer.has_value()) {
                    return;
                }
                const auto& buffer = opt_buffer.value();
                this->bind_buffer(context, buffer);
            } break;
            case types::RenderHandleType::Shader: {
                this->bind_shader(context, id);
            } break;
            case types::RenderHandleType::Mesh: {
                this->bind_mesh(context, id);
            } break;
            case types::RenderHandleType::Texture: {
            } break;
            case types::RenderHandleType::View: {
                this->bind_render_target(context, id);
            } break;
            case types::RenderHandleType::Rasterizer: {
                this->bind_rasterizer(context, id);
            } break;
            case types::RenderHandleType::Topology: {
                this->bind_topology(context, id);
            } break;
            case types::RenderHandleType::InputLayout: {
                this->bind_input_layout(context, id);
            } break;
            default:
                break;
        }
    }

    void D3D11Renderer::bind_buffer(
        ID3D11DeviceContext* const context, const Buffer& buffer) const {
        if (const auto vertex = std::get_if<VertexParameter>(&buffer.parameter)) {
            context->IASetVertexBuffers(vertex->target_slot,
                1,
                buffer.buffer.GetAddressOf(),
                &vertex->stride,
                &vertex->offset);
        } else if (const auto index = std::get_if<IndexParameter>(&buffer.parameter)) {
            context->IASetIndexBuffer(buffer.buffer.Get(), index->format, index->offset);
        } else if (const auto uniform = std::get_if<UniformParameter>(&buffer.parameter)) {
            switch (uniform->target_shader) {
                case ShaderType::Vertex: {
                    context->VSSetConstantBuffers(
                        uniform->target_slot, 1, buffer.buffer.GetAddressOf());
                } break;
                case ShaderType::Pixcel: {
                    context->PSSetConstantBuffers(
                        uniform->target_slot, 1, buffer.buffer.GetAddressOf());
                } break;
                case ShaderType::Compute: {
                    context->CSSetConstantBuffers(
                        uniform->target_slot, 1, buffer.buffer.GetAddressOf());
                } break;
                default:
                    break;
            }
        }
    }

    void D3D11Renderer::bind_shader(
        ID3D11DeviceContext* const context, const types::HandleId id) const {
        const auto& pool = this->resource_manager.get_shader_pool();
        const auto opt_type = pool.get_shader_type(id);
        if (opt_type.is_none()) {
            return;
        }

        switch (opt_type.value()) {
            case ShaderType::Vertex: {
                const auto opt_shader = pool.get_vertex_shader(id);
                if (opt_shader.is_none()) {
                    return;
                }
                context->VSSetShader(opt_shader.value(), nullptr, 0);
            } break;
            case ShaderType::Pixcel: {
                const auto opt_shader = pool.get_pixel_shader(id);
                if (opt_shader.is_none()) {
                    return;
                }
                context->PSSetShader(opt_shader.value(), nullptr, 0);
            } break;
            case ShaderType::Compute: {
                const auto opt_shader = pool.get_compute_shader(id);
                if (opt_shader.is_none()) {
                    return;
                }
                context->CSSetShader(opt_shader.value(), nullptr, 0);
            } break;
            default:
                break;
        }
    }

    void D3D11Renderer::bind_render_target(
        ID3D11DeviceContext* const context, const types::HandleId id) const {
        const auto& view_pool = this->resource_manager.get_view_pool();
        const auto opt_type = view_pool.get_view_type(id);
        if (opt_type.is_none()) {
            return;
        }

        switch (opt_type.value()) {
            case types::ImageViewType::DepthStencil: {
                const auto opt_view = view_pool.get_depth_stencil_view(id);
                if (opt_view.is_none()) {
                    return;
                }

                context->OMSetRenderTargets(1, nullptr, opt_view.value());
            } break;
            case types::ImageViewType::RenderTarget: {
                const auto opt_view = view_pool.get_address_render_target_view(id);
                if (opt_view.is_none()) {
                    return;
                }
                context->OMSetRenderTargets(1, opt_view.value(), nullptr);
            } break;
            case types::ImageViewType::ShaderResource: {
            } break;
            case types::ImageViewType::UnorderedAccess: {
            } break;
            default:
                break;
        }
    }

    void D3D11Renderer::bind_rasterizer(
        ID3D11DeviceContext* const context, const types::HandleId id) const {
        const auto opt_rasterizer = this->resource_manager.get_rasterizer(id);
        if (opt_rasterizer.is_none()) {
            return;
        }

        const auto& rasterizer = opt_rasterizer.unwrap();
        context->RSSetState(rasterizer.Get());
    }

    void D3D11Renderer::bind_mesh(
        ID3D11DeviceContext* const context, const types::HandleId id) const {
        const auto& opt_mesh = this->resource_manager.get_mesh(id);
        if (opt_mesh.is_none()) {
            return;
        }
        const auto& mesh = opt_mesh.unwrap();

        for (const auto handle : mesh.mesh_handles) {
            this->bind_handle(context, handle);
        }
    }

    void D3D11Renderer::bind_topology(
        ID3D11DeviceContext* const context, const types::HandleId id) const {
        const auto topology = static_cast<types::PrimitiveTopology>(id);
        const auto d3d11_topology = D3D11Converter::to_topology(topology);
        if (d3d11_topology == D3D11_PRIMITIVE_TOPOLOGY::D3D11_PRIMITIVE_TOPOLOGY_UNDEFINED) {
            return;
        }

        context->IASetPrimitiveTopology(d3d11_topology);
    }

    void D3D11Renderer::bind_input_layout(
        ID3D11DeviceContext* const context, const types::HandleId id) const {
        const auto opt_input_layout = this->resource_manager.get_input_layout(id);
        if (opt_input_layout.is_none()) {
            return;
        }

        const auto& input_layout = opt_input_layout.unwrap();
        context->IASetInputLayout(input_layout.Get());
    }

    D3D11Renderer::D3D11Renderer(std::unique_ptr<D3D11> d3d11)
        : d3d11(std::move(d3d11))
        , resource_manager(ResourceManager{}) {
    }

    platform::RenderResult<types::RenderPass> D3D11Renderer::create_render_pass(
        const types::PipelineDescription& description) {
        types::RenderPass pass{};

        // トポロジの追加
        pass.commands.emplace_back(types::RenderHandle{
            .id = static_cast<types::HandleId>(description.topology),
            .type = types::RenderHandleType::Topology,
        });

        // ラスタライザの追加
        pass.commands.emplace_back(description.rasterizer);

        // シェーダーの追加
        const auto& shader_pool = this->resource_manager.get_shader_pool();
        for (const auto& shader : description.shaders) {
            pass.commands.emplace_back(shader);
        }

        return pass;
    }

    platform::RenderResult<types::RenderHandle> D3D11Renderer::create_viewport(
        const types::ViewportRect& config) {
        const auto result = this->resource_manager.make_viewport(config);
        if (result.is_err()) {
            return result.propagation(platform::RenderError::MakeError);
        }
        return result.value();
    }

    platform::RenderResult<std::unique_ptr<platform::IPipelineLayout>>
    D3D11Renderer::create_pipeline_layout(const types::VertexLayout& layout,
        const types::RenderHandle& vertex_shader,
        const types::RenderHandle& pixel_shader) {
        return platform::RenderResult<std::unique_ptr<platform::IPipelineLayout>>();
    }

    platform::RenderResult<types::RenderHandle> D3D11Renderer::create_pipeline_layout_from_shader(
        const types::ShaderData& shader_data) {
        const auto device = this->d3d11->get_device();
        const auto result =
            this->resource_manager.make_input_layout_from_shader(device, shader_data);
        if (result.is_err()) {
            return result.propagation(platform::RenderError::MakeError);
        }

        return result.value();
    }

    platform::RenderResult<types::RenderHandle> D3D11Renderer::create_rasterizer(
        const types::RasterizerDescription& description) {
        const auto device = this->d3d11->get_device();
        const auto result = this->resource_manager.make_rasterizer(device, description);
        if (result.is_err()) {
            return result.propagation(platform::RenderError::MakeError);
        }

        return result.value();
    }

    platform::RenderResult<types::RenderHandle> D3D11Renderer::create_image(
        const types::ImageDescription& description) {
        const auto device = this->d3d11->get_device();
        const auto result = this->resource_manager.make_image(device, description);
        if (result.is_err()) {
            return result.propagation(platform::RenderError::MakeError);
        }

        return result.value();
    }

    platform::RenderResult<std::weak_ptr<platform::IRenderTargetView>>
    D3D11Renderer::create_render_target_view(
        types::RenderHandle image_handle, const types::ImageViewDescription& description) {
        const auto device = this->d3d11->get_device();
        const auto result =
            this->resource_manager.make_render_target_view(device, image_handle, description);
        if (result.is_err()) {
            return result.propagation(platform::RenderError::MakeError);
        }

        const auto rtv = std::make_shared<RenderTargetView>(result.value(), description);
        this->render_targets.emplace_back(rtv);

        return rtv;
    }

    platform::RenderResult<std::weak_ptr<platform::IDepthStencilView>>
    D3D11Renderer::create_depth_stencil_view(
        types::RenderHandle image_handle, const types::ImageViewDescription& description) {
        return foundation::Error(platform::RenderError::MakeError);
    }

    platform::RenderResult<std::weak_ptr<platform::IShaderResourceView>>
    D3D11Renderer::create_shader_resource_view(
        types::RenderHandle image_handle, const types::ImageViewDescription& description) {
        return foundation::Error(platform::RenderError::MakeError);
    }

    platform::RenderResult<std::weak_ptr<platform::IUnorderedAccessView>>
    D3D11Renderer::create_unordered_access_view(
        types::RenderHandle image_handle, const types::ImageViewDescription& description) {
        return foundation::Error(platform::RenderError::MakeError);
    }

    platform::RenderResult<types::RenderHandle> D3D11Renderer::create_mesh(
        const types::MeshData& mesh) {
        const auto device = this->d3d11->get_device();
        const auto result = this->resource_manager.make_mesh(device, mesh);
        if (result.is_err()) {
            return result.propagation(platform::RenderError::MakeError);
        }

        return result.value();
    }

    platform::RenderResult<types::RenderHandle> D3D11Renderer::create_texture(
        const types::TextureData& texture) {
        const auto device = this->d3d11->get_device();
        const auto result = this->resource_manager.make_texture(device, texture);
        if (result.is_err()) {
            return result.propagation(platform::RenderError::MakeError);
        }

        return result.value();
    }

    platform::RenderResult<types::RenderHandle> D3D11Renderer::create_shader(
        const types::ShaderKind kind, const types::ShaderData& shader_data) {
        const auto device = this->d3d11->get_device();
        const auto result = this->resource_manager.make_shader(device, shader_data);
        if (result.is_err()) {
            return result.propagation(platform::RenderError::MakeError);
        }

        return result.value();
    }

    void D3D11Renderer::submit_render_graph(const types::RenderGraph& graph) {
        const auto& context = this->d3d11->get_context();
        const auto& view_pool = this->resource_manager.get_view_pool();

        // ビューポートのセット
        const auto& viewports = this->resource_manager.get_viewports();
        context->RSSetViewports(viewports.size(), viewports.data());

        // レンダーターゲットのクリア
        for (const auto& render_target : this->render_targets) {
            const auto handle = render_target->get_handle();
            const auto opt_target = view_pool.get_render_target_view(handle.id);
            if (opt_target.is_none()) {
                continue;
            }
            const auto target = opt_target.value();

            const auto color = render_target->get_clear_color();
            const float clear_color[4] = {color.r, color.g, color.b, color.a};
            context->ClearRenderTargetView(target, clear_color);
        }

        // 各パイプラインに応じた描画コマンド実行
        for (const auto& pass : graph.passes) {
            for (const auto& command : pass.commands) {
                this->execute(context, command);
            }
        }
    }

    void D3D11Renderer::present(void) {
        this->d3d11->get_swap_chain()->Present(1, 0);
    }
} // namespace enishi::renderer::directx