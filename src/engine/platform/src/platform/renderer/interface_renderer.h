#pragma once
#include "interface_pipeline_layout.h"
#include "interface_render_handle_mapper.h"
#include "interface_render_resource_accessor.h"
#include "updater/interface_resource_updater.h"
#include "view/interface_image_view.h"
#include <engine_types/assets/model/model_data.h>
#include <engine_types/assets/shader/shader_data.h>
#include <engine_types/assets/shader/shader_kind.h>
#include <engine_types/assets/texture/texture_data.h>
#include <engine_types/handle/renderer/handles/resource_handles.h>
#include <engine_types/handle/renderer/render_handle.h>
#include <engine_types/renderer/description/pipeline_description.h>
#include <engine_types/renderer/description/rasterizer/rasterizer_description.h>
#include <engine_types/renderer/description/sampler/sampler_description.h>
#include <engine_types/renderer/description/view/image_description.h>
#include <engine_types/renderer/description/view/image_view_description.h>
#include <engine_types/renderer/render_graph.h>
#include <engine_types/renderer/viewport.h>
#include <engine_types/window/window_types.h>
#include <foundation/option/option.h>
#include <foundation/result/result.h>
#include <memory>
#include <platform/errors/renderer_errors.h>
#include <vector>

namespace enishi::platform {
    template <typename T> using RenderResult = foundation::Result<T, RenderError>;

    // 主に作ったり描画を担当する
    // 細かい設定は別のインターフェイスが担当する
    class IRenderer {
      public:
        virtual ~IRenderer(void) noexcept = default;

        [[nodiscard]]
        virtual RenderResult<types::RenderHandle> create_viewport(
            const types::ViewportRect& config) = 0;

        [[nodiscard]]
        virtual RenderResult<types::RenderHandle> create_shader_reflection(
            const types::ShaderData& shader_data) = 0;

        [[nodiscard]]
        virtual RenderResult<std::unique_ptr<IPipelineLayout>> create_vertex_layout(
            const types::VertexLayout& layout,
            const types::RenderHandle& vertex_shader,
            const types::RenderHandle& pixel_shader) = 0;

        [[nodiscard]]
        virtual RenderResult<types::RenderHandle> create_vertex_layout_from_shader_data(
            const types::ShaderData& shader_data) = 0;

        [[nodiscard]]
        virtual RenderResult<types::RenderHandle> create_rasterizer(
            const types::RasterizerStateDescription& description) = 0;

        [[nodiscard]]
        virtual RenderResult<types::RenderHandle> create_sampler(
            const types::SamplerStateDescription& description) = 0;

        [[nodiscard]]
        virtual RenderResult<types::RenderHandle> create_depth_stencil(
            const types::DepthStencilStateDescription& description) = 0;

        [[nodiscard]]
        virtual RenderResult<types::RenderHandle> create_blend(
            const types::BlendStateDescription& description) = 0;

        [[nodiscard]]
        virtual RenderResult<types::RenderHandle> create_image(
            const types::ImageDescription& description) = 0;

        [[nodiscard]]
        virtual RenderResult<std::shared_ptr<IRenderTargetView>> create_render_target_view(
            types::RenderHandle image_handle, const types::ImageViewDescription& description) = 0;

        [[nodiscard]]
        virtual RenderResult<std::shared_ptr<IDepthStencilView>> create_depth_stencil_view(
            types::RenderHandle image_handle, const types::ImageViewDescription& description) = 0;

        [[nodiscard]]
        virtual RenderResult<std::shared_ptr<IShaderResourceView>> create_shader_resource_view(
            types::RenderHandle image_handle, const types::ImageViewDescription& description) = 0;

        [[nodiscard]]
        virtual RenderResult<std::shared_ptr<IUnorderedAccessView>> create_unordered_access_view(
            types::RenderHandle image_handle, const types::ImageViewDescription& description) = 0;

        [[nodiscard]]
        virtual RenderResult<types::RenderHandle> create_mesh(const types::ModelData& model_data,
            const std::vector<types::RenderHandle>& shader_reflections) = 0;

        [[nodiscard]]
        virtual RenderResult<types::RenderHandle> create_texture(
            const types::TextureData& texture) = 0;

        [[nodiscard]]
        virtual RenderResult<types::RenderHandle> create_shader(
            const types::ShaderKind kind, const types::ShaderData& shader_data) = 0;

        [[nodiscard]]
        virtual IRenderResourceAccessor* get_resource_accessor(void) noexcept = 0;

        [[nodiscard]]
        virtual IRenderResourceAccessor* const get_resource_accessor(void) const noexcept = 0;

        [[nodiscard]]
        virtual const IRenderHandleMapper* get_handle_mapper(void) const noexcept = 0;
    };
} // namespace enishi::platform