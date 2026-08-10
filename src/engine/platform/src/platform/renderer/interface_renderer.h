#pragma once
#include "../errors/renderer_errors.h"
#include "interface_image_view.h"
#include "interface_pipeline_layout.h"
#include <engine_types/assets/shader/shader_data.h>
#include <engine_types/assets/shader/shader_kind.h>
#include <engine_types/assets/texture/texture_data.h>
#include <engine_types/renderer/description/image_description.h>
#include <engine_types/renderer/description/image_view_description.h>
#include <engine_types/renderer/description/pipeline_description.h>
#include <engine_types/renderer/description/rasterizer_description.h>
#include <engine_types/renderer/description/sampler_description.h>
#include <engine_types/renderer/mesh_data.h>
#include <engine_types/renderer/render_graph.h>
#include <engine_types/renderer/render_handle.h>
#include <engine_types/renderer/viewport.h>
#include <engine_types/window/window_types.h>
#include <foundation/result/result.h>
#include <memory>

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
            std::shared_ptr<types::ShaderData> shader_data) = 0;

        [[nodiscard]]
        virtual RenderResult<std::unique_ptr<IPipelineLayout>> create_pipeline_layout(
            const types::VertexLayout& layout,
            const types::RenderHandle& vertex_shader,
            const types::RenderHandle& pixel_shader) = 0;

        [[nodiscard]]
        virtual RenderResult<types::RenderHandle> create_pipeline_layout_from_shader_reflection(
            const types::RenderHandle& shader_reflection_handle) = 0;

        [[nodiscard]]
        virtual RenderResult<types::RenderHandle> create_rasterizer(
            const types::RasterizerDescription& description) = 0;

        [[nodiscard]]
        virtual RenderResult<types::RenderHandle> create_image(
            const types::ImageDescription& description) = 0;

        [[nodiscard]]
        virtual RenderResult<std::weak_ptr<IRenderTargetView>> create_render_target_view(
            types::RenderHandle image_handle, const types::ImageViewDescription& description) = 0;

        [[nodiscard]]
        virtual RenderResult<std::weak_ptr<IDepthStencilView>> create_depth_stencil_view(
            types::RenderHandle image_handle, const types::ImageViewDescription& description) = 0;

        [[nodiscard]]
        virtual RenderResult<std::weak_ptr<IShaderResourceView>> create_shader_resource_view(
            types::RenderHandle image_handle, const types::ImageViewDescription& description) = 0;

        [[nodiscard]]
        virtual RenderResult<std::weak_ptr<IUnorderedAccessView>> create_unordered_access_view(
            types::RenderHandle image_handle, const types::ImageViewDescription& description) = 0;

        [[nodiscard]]
        virtual RenderResult<types::RenderHandle> create_mesh(types::MeshData&& mesh) = 0;

        [[nodiscard]]
        virtual RenderResult<types::RenderHandle> create_texture(
            const types::TextureData& texture) = 0;

        [[nodiscard]]
        virtual RenderResult<types::RenderHandle> create_shader(
            const types::ShaderKind kind, const types::ShaderData& shader_data) = 0;
    };
} // namespace enishi::platform