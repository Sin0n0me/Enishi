#include "d3d11_renderer.h"
#include "d3d11_converter.h"
#include "shader/shader_refrection.h"
#include "view/render_target_view.h"
#include <ranges>

namespace enishi::renderer::directx {
    D3D11Renderer::D3D11Renderer(std::unique_ptr<D3D11> d3d11)
        : d3d11(std::move(d3d11)) {
        this->resource_manager = std::make_unique<ResourceManager>(this->d3d11);
    }

    platform::RenderResult<types::RenderPass> D3D11Renderer::create_render_pass(
        const types::PipelineDescription& description) {
        types::RenderPass pass{};

        // RTVの追加
        pass.render_target = description.render_target;

        // トポロジの追加
        pass.commands.emplace_back(types::RenderHandle{
            .id = static_cast<types::HandleId>(description.topology),
            .type = types::RenderHandleType::Topology,
        });

        // ラスタライザの追加
        pass.commands.emplace_back(description.rasterizer);

        // 頂点レイアウトの追加
        pass.commands.emplace_back(description.vertex_layout);

        // シェーダーの追加
        const auto& shader_pool = this->resource_manager->get_shader_pool();
        for (const auto& shader : description.shaders) {
            pass.commands.emplace_back(shader);
        }

        return pass;
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
    D3D11Renderer::create_pipeline_layout(const types::VertexLayout& layout,
        const types::RenderHandle& vertex_shader,
        const types::RenderHandle& pixel_shader) {
        return std::unique_ptr<platform::IPipelineLayout>{};
    }

    platform::RenderResult<types::RenderHandle> D3D11Renderer::create_pipeline_layout_from_shader(
        const types::ShaderData& shader_data) {
        const auto result = this->resource_manager->make_input_layout_from_shader(shader_data);
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

    platform::RenderResult<std::weak_ptr<platform::IRenderTargetView>>
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
        const auto result = this->resource_manager->make_mesh(mesh);
        if (result.is_err()) {
            return result.propagation(platform::RenderError::MakeError);
        }

        return result.unwrap();
    }

    platform::RenderResult<types::RenderHandle> D3D11Renderer::create_texture(
        const types::TextureData& texture) {
        const auto result = this->resource_manager->make_texture(texture);
        if (result.is_err()) {
            return result.propagation(platform::RenderError::MakeError);
        }

        return result.unwrap();
    }

    platform::RenderResult<types::RenderHandle> D3D11Renderer::create_shader(
        const types::ShaderKind kind, const types::ShaderData& shader_data) {
        const auto result = this->resource_manager->make_shader(kind, shader_data);
        if (result.is_err()) {
            return result.propagation(platform::RenderError::MakeError);
        }

        return result.unwrap();
    }

    void D3D11Renderer::setup_viewports(void) const {
        // ビューポートのセット
        const auto& context = this->d3d11->get_context();
        const auto& viewports = this->resource_manager->get_viewports();
        context->RSSetViewports(viewports.size(), viewports.data());
    }

    void D3D11Renderer::setup_render_targets(void) const {
        // レンダーターゲットのクリア
        const auto& context = this->d3d11->get_context();
        const auto& view_pool = this->resource_manager->get_view_pool();
        const auto& accessor = this->resource_manager->get_accessor();

        for (const auto& render_target : accessor->get_render_targets()) {
            const auto handle = render_target->get_handle();
            const auto opt_target = view_pool.get_render_target_view(handle.id);
            if (opt_target.is_none()) {
                continue;
            }
            auto&& target = opt_target.unwrap();

            const auto color = render_target->get_clear_color();
            const float clear_color[4] = {color.r, color.g, color.b, color.a};
            context->ClearRenderTargetView(target.Get(), clear_color);
        }
    }

    void D3D11Renderer::bind_buffer(const types::HandleId id) const {
        const auto& opt_buffer = this->resource_manager->get_buffer(id);
        if (!opt_buffer.has_value()) {
            return;
        }
        const auto& buffer = opt_buffer.unwrap();

        const auto context = this->d3d11->get_context();
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

    void D3D11Renderer::bind_shader(const types::HandleId id) const {
        const auto& pool = this->resource_manager->get_shader_pool();
        const auto opt_type = pool.get_shader_type(id);
        if (opt_type.is_none()) {
            return;
        }

        const auto context = this->d3d11->get_context();
        switch (opt_type.value()) {
            case types::ShaderKind::Vertex: {
                const auto opt_shader = pool.get_vertex_shader(id);
                if (opt_shader.is_none()) {
                    return;
                }
                context->VSSetShader(opt_shader.value().Get(), nullptr, 0);
            } break;
            case types::ShaderKind::Pixel: {
                const auto opt_shader = pool.get_pixel_shader(id);
                if (opt_shader.is_none()) {
                    return;
                }
                context->PSSetShader(opt_shader.value().Get(), nullptr, 0);
            } break;
            case types::ShaderKind::Compute: {
                const auto opt_shader = pool.get_compute_shader(id);
                if (opt_shader.is_none()) {
                    return;
                }
                context->CSSetShader(opt_shader.value().Get(), nullptr, 0);
            } break;
            default:
                break;
        }
    }

    void D3D11Renderer::bind_texture(const types::HandleId id) const {
    }

    void D3D11Renderer::bind_view(const types::HandleId id) const {
        const auto& view_pool = this->resource_manager->get_view_pool();
        const auto opt_type = view_pool.get_view_type(id);
        if (opt_type.is_none()) {
            return;
        }

        const auto context = this->d3d11->get_context();
        switch (opt_type.value()) {
            case types::ImageViewType::DepthStencil: {
                const auto opt_view = view_pool.get_depth_stencil_view(id);
                if (opt_view.is_none()) {
                    return;
                }

                context->OMSetRenderTargets(1, nullptr, opt_view.unwrap().Get());
            } break;
            case types::ImageViewType::RenderTarget: {
                const auto opt_view = view_pool.get_render_target_view(id);
                if (opt_view.is_none()) {
                    return;
                }
                context->OMSetRenderTargets(1, opt_view.unwrap().GetAddressOf(), nullptr);
            } break;
            case types::ImageViewType::ShaderResource: {
            } break;
            case types::ImageViewType::UnorderedAccess: {
            } break;
            default:
                break;
        }
    }

    void D3D11Renderer::bind_rasterizer(const types::HandleId id) const {
        const auto opt_rasterizer = this->resource_manager->get_rasterizer(id);
        if (opt_rasterizer.is_none()) {
            return;
        }

        const auto context = this->d3d11->get_context();
        const auto& rasterizer = opt_rasterizer.unwrap();
        context->RSSetState(rasterizer.Get());
    }

    void D3D11Renderer::bind_mesh(const types::HandleId id) const {
        const auto& opt_mesh = this->resource_manager->get_mesh(id);
        if (opt_mesh.is_none()) {
            return;
        }
        const auto& mesh = opt_mesh.unwrap();

        for (const auto handle : mesh.mesh_handles) {
            switch (handle.type) {
                case types::RenderHandleType::Buffer: {
                    this->bind_buffer(handle.id);
                } break;
                case types::RenderHandleType::Texture: {
                    this->bind_texture(handle.id);
                } break;
                default:
                    break;
            }
        }
    }

    void D3D11Renderer::bind_topology(const types::HandleId id) const {
        const auto topology = static_cast<types::PrimitiveTopology>(id);
        const auto d3d11_topology = D3D11Converter::to_topology(topology);
        if (d3d11_topology == D3D11_PRIMITIVE_TOPOLOGY::D3D11_PRIMITIVE_TOPOLOGY_UNDEFINED) {
            return;
        }

        const auto context = this->d3d11->get_context();
        context->IASetPrimitiveTopology(d3d11_topology);
    }

    void D3D11Renderer::bind_input_layout(const types::HandleId id) const {
        const auto opt_input_layout = this->resource_manager->get_input_layout(id);
        if (opt_input_layout.is_none()) {
            return;
        }

        const auto context = this->d3d11->get_context();
        const auto& input_layout = opt_input_layout.unwrap();
        context->IASetInputLayout(input_layout.Get());
    }

    void D3D11Renderer::present(void) const {
        this->d3d11->get_swap_chain()->Present(1, 0);
    }
} // namespace enishi::renderer::directx