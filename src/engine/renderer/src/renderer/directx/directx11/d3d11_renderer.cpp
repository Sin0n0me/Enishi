#include "d3d11_renderer.h"
#include "resource/view/render_target_view.h"
#include <renderer/common/converter/model_to_mesh.h>
#include <renderer/directx/directx11/d3d11_render_command_encoder.h>
#include <renderer/directx/directx11/resource/d3d11_render_resources.h>

namespace enishi::renderer::directx {

    D3D11Renderer::D3D11Renderer(std::unique_ptr<D3D11> context)
        : resources(std::make_shared<D3D11RenderResources>(std::move(context)))
        , encoder(std::make_shared<D3D11RenderCommandEncoder>(this->resources)) {
    }

    std::shared_ptr<ID3D11Context> D3D11Renderer::get_native_context(void) const {
        return this->resources->get_context();
    }

    platform::RenderResult<types::RenderHandle> D3D11Renderer::create_viewport(
        const types::ViewportRect& config) {
        const auto result = this->resources->get_resource_manager().make_viewport(config);
        if (result.is_err()) {
            return result.propagation(platform::RenderError::MakeError);
        }
        return result.unwrap();
    }

    platform::RenderResult<std::unique_ptr<platform::IPipelineLayout>>
    D3D11Renderer::create_vertex_layout(const types::VertexLayout& layout,
        const types::RenderHandle& vertex_shader,
        const types::RenderHandle& pixel_shader) {
        return foundation::Error(platform::RenderError::MakeError,
            "D3D11 vertex layout creation from a layout description is not implemented");
    }

    platform::RenderResult<types::RenderHandle> D3D11Renderer::create_shader_reflection(
        const types::ShaderData& shader_data) {
        const auto result =
            this->resources->get_resource_manager().make_shader_reflection(shader_data);
        if (result.is_err()) {
            return result.propagation(platform::RenderError::MakeError);
        }

        return result.unwrap();
    }

    platform::RenderResult<types::RenderHandle>
    D3D11Renderer::create_vertex_layout_from_shader_data(const types::ShaderData& shader_data) {
        const auto result = this->resources->get_resource_manager()
                                .make_input_layout_from_shader_data(shader_data)
                                .add_message("シェーダーから頂点レイアウトを作成できませんでした");
        if (result.is_err()) {
            return result.propagation(platform::RenderError::MakeError);
        }

        return result.unwrap();
    }

    platform::RenderResult<types::RenderHandle> D3D11Renderer::create_rasterizer(
        const types::RasterizerStateDescription& description) {
        const auto result = this->resources->get_resource_manager()
                                .make_rasterizer_state(description)
                                .add_message("ラスタライザステートの作成に失敗しました");
        if (result.is_err()) {
            return result.propagation(platform::RenderError::MakeError);
        }

        return result.unwrap();
    }

    platform::RenderResult<types::RenderHandle> D3D11Renderer::create_sampler(
        const types::SamplerStateDescription& description) {
        const auto result = this->resources->get_resource_manager()
                                .make_sampler_state(description)
                                .add_message("サンプラーステートの作成に失敗しました");
        if (result.is_err()) {
            return result.propagation(platform::RenderError::MakeError);
        }
        const auto& handle = result.unwrap();

        return result.unwrap();
    }

    platform::RenderResult<types::RenderHandle> D3D11Renderer::create_depth_stencil(
        const types::DepthStencilStateDescription& description) {
        const auto result = this->resources->get_resource_manager()
                                .make_depth_stencil_state(description)
                                .add_message("サンプラーステートの作成に失敗しました");
        if (result.is_err()) {
            return result.propagation(platform::RenderError::MakeError);
        }
        const auto& handle = result.unwrap();

        return result.unwrap();
    }

    platform::RenderResult<types::RenderHandle> D3D11Renderer::create_blend(
        const types::BlendStateDescription& description) {
        const auto result = this->resources->get_resource_manager()
                                .make_blend_state(description)
                                .add_message("ブレンドステートの作成に失敗しました");
        if (result.is_err()) {
            return result.propagation(platform::RenderError::MakeError);
        }
        const auto& handle = result.unwrap();

        return result.unwrap();
    }

    platform::RenderResult<types::RenderHandle> D3D11Renderer::create_image(
        const types::ImageDescription& description) {
        const auto result = this->resources->get_resource_manager().make_image(description);
        if (result.is_err()) {
            return result.propagation(platform::RenderError::MakeError);
        }

        return result.unwrap();
    }

    platform::RenderResult<std::shared_ptr<platform::IRenderTargetView>>
    D3D11Renderer::create_render_target_view(
        types::RenderHandle image_handle, const types::ImageViewDescription& description) {
        const auto result = this->resources->get_resource_manager()
                                .make_view(image_handle, description)
                                .add_message("ビューの作成に失敗しました");
        if (result.is_err()) {
            return result.propagation(platform::RenderError::MakeError);
        }
        const auto& handle = result.unwrap();
        const auto opt_index =
            this->resources->get_resource_manager().get_render_handle_mapper()->get(handle);
        if (opt_index.is_none()) {
            return foundation::Error(
                platform::RenderError::MakeError, "Render target view handle mapping is missing");
        }
        const auto& index = opt_index.unwrap();
        auto opt_rtv = this->resources->get_resource_manager()
                           .get_resource_accessor()
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
        const auto result = this->resources->get_resource_manager()
                                .make_view(image_handle, description)
                                .add_message("ビューの作成に失敗しました");
        if (result.is_err()) {
            return result.propagation(platform::RenderError::MakeError);
        }
        const auto& handle = result.unwrap();
        const auto opt_index =
            this->resources->get_resource_manager().get_render_handle_mapper()->get(handle);
        if (opt_index.is_none()) {
            return foundation::Error(
                platform::RenderError::MakeError, "Depth stencil view handle mapping is missing");
        }
        const auto& index = opt_index.unwrap();
        auto opt_rtv = this->resources->get_resource_manager()
                           .get_resource_accessor()
                           ->get_view_accessor()
                           ->get_depth_stencil_view(index.configurable);
        if (opt_rtv.is_none()) {
            return foundation::Error(
                platform::RenderError::MakeError, "Depth stencil view resource is missing");
        }

        return opt_rtv.unwrap();
    }

    platform::RenderResult<std::shared_ptr<platform::IShaderResourceView>>
    D3D11Renderer::create_shader_resource_view(
        types::RenderHandle image_handle, const types::ImageViewDescription& description) {
        const auto result = this->resources->get_resource_manager()
                                .make_view(image_handle, description)
                                .add_message("ビューの作成に失敗しました");
        if (result.is_err()) {
            return result.propagation(platform::RenderError::MakeError);
        }
        const auto& handle = result.unwrap();
        const auto opt_index =
            this->resources->get_resource_manager().get_render_handle_mapper()->get(handle);
        if (opt_index.is_none()) {
            return foundation::Error(
                platform::RenderError::MakeError, "Shader resource view handle mapping is missing");
        }
        const auto& index = opt_index.unwrap();
        auto opt_rtv = this->resources->get_resource_manager()
                           .get_resource_accessor()
                           ->get_view_accessor()
                           ->get_shader_resource_view(index.configurable);
        if (opt_rtv.is_none()) {
            return foundation::Error(
                platform::RenderError::MakeError, "Shader resource view is missing");
        }

        return opt_rtv.unwrap();
    }

    platform::RenderResult<std::shared_ptr<platform::IUnorderedAccessView>>
    D3D11Renderer::create_unordered_access_view(
        types::RenderHandle image_handle, const types::ImageViewDescription& description) {
        const auto result = this->resources->get_resource_manager()
                                .make_view(image_handle, description)
                                .add_message("ビューの作成に失敗しました");
        if (result.is_err()) {
            return result.propagation(platform::RenderError::MakeError);
        }
        const auto& handle = result.unwrap();
        const auto opt_index =
            this->resources->get_resource_manager().get_render_handle_mapper()->get(handle);
        if (opt_index.is_none()) {
            return foundation::Error(platform::RenderError::MakeError,
                "Unordered access view handle mapping is missing");
        }
        const auto& index = opt_index.unwrap();
        auto opt_rtv = this->resources->get_resource_manager()
                           .get_resource_accessor()
                           ->get_view_accessor()
                           ->get_unordered_access_view(index.configurable);
        if (opt_rtv.is_none()) {
            return foundation::Error(
                platform::RenderError::MakeError, "Unordered access view resource is missing");
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
        const auto result = this->resources->get_resource_manager()
                                .make_mesh(std::move(mesh_result).unwrap_mut(), shader_reflections)
                                .add_message("メッシュの作成に失敗しました");
        if (result.is_err()) {
            return result.propagation(platform::RenderError::MakeError);
        }

        return result.unwrap();
    }

    platform::RenderResult<types::RenderHandle> D3D11Renderer::create_texture(
        const types::TextureData& texture) {
        const auto result =
            this->resources->get_resource_manager().make_texture(texture).add_message(
                "テクスチャの作成に失敗しました");
        if (result.is_err()) {
            return result.propagation(platform::RenderError::MakeError);
        }

        return result.unwrap();
    }

    platform::RenderResult<types::RenderHandle> D3D11Renderer::create_shader(
        const types::ShaderKind kind, const types::ShaderData& shader_data) {
        const auto result = this->resources->get_resource_manager()
                                .make_shader(kind, shader_data)
                                .add_message("シェーダーの作成に失敗しました");
        if (result.is_err()) {
            return result.propagation(platform::RenderError::MakeError);
        }

        return result.unwrap();
    }

    platform::IRenderResourceAccessor* D3D11Renderer::get_resource_accessor(void) noexcept {
        return &this->resources->get_resource_manager();
    }

    platform::IRenderResourceAccessor* const D3D11Renderer::get_resource_accessor(
        void) const noexcept {
        return &this->resources->get_resource_manager();
    }

    const platform::IRenderHandleMapper* D3D11Renderer::get_handle_mapper(void) const noexcept {
        return this->resources->get_resource_manager().get_render_handle_mapper();
    }

    D3D11Renderer::~D3D11Renderer() noexcept = default;

    std::shared_ptr<platform::IRenderCommandEncoder>
    D3D11Renderer::get_command_encoder() const noexcept {
        return this->encoder;
    }
} // namespace enishi::renderer::directx
