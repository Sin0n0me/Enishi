#include "helper.h"
#include "shadow_map_render_pass_constructor.h"
#include <foundation/log/logger.h>
#include <foundation/path/path_utility.h>
#include <foundation/str/string_builder.h>
#include <renderer/common/render_pass/render_pass.h>
#include <settings.h>

namespace enishi {
    constexpr char VS_FILE_NAME[] = "vs_shadow_map";
    constexpr char PS_FILE_NAME[] = "ps_shadow_map";
    constexpr std::uint32_t SHADOW_MAP_SIZE = 2048;

    foundation::Result<std::shared_ptr<platform::IRenderPass>, core::SystemError>
    ShadowMapRenderPassConstructor::make(
        platform::IRenderer* const renderer, assets_system::IAssetSystem* const asset_system) {
        auto render_pass = std::make_shared<renderer::RenderPass>();

        types::PipelineDescription description{
            .topology = types::PrimitiveTopology::TriangleList,
        };

        // レンダーターゲットの作成
        auto rtv = make_render_target(types::ImageDescription::make_render_target(WINDOW_SIZE),
            types::ImageFormat::BGRA8_UNORM,
            renderer);
        if (rtv.is_err()) {
            return std::move(rtv).unwrap_err();
        }
        description.render_target_view = rtv.unwrap();

        // 深度ステンシルの作成
        auto dsv = make_depth_stencil(types::ImageDescription::make_depth_stencil(
                                          WINDOW_SIZE, types::ImageFormat::D24_UNORM_S8_UINT),
            types::ImageFormat::BGRA8_UNORM,
            renderer);
        if (dsv.is_err()) {
            return std::move(dsv).unwrap_err();
        }
        description.depth_stencil_view = dsv.unwrap();

        // ラスタライザの作成
        auto rasterizer = make_rasterizer(
            types::RasterizerStateDescription{
                .cull_mode = types::CullMode::None,
                .front_face = types::FrontFace::CounterClockwise,
            },
            renderer);
        if (rasterizer.is_err()) {
            return std::move(rasterizer).unwrap_err();
        }
        description.rasterizer_state = rasterizer.unwrap();

        // シェーダーの作成
        ShaderPaths paths = {
            {types::ShaderKind::Vertex, {VS_FILE_NAME}},
            {types::ShaderKind::Pixel, {PS_FILE_NAME}},
        };
        auto shader_result = make_shaders(renderer, asset_system, std::move(paths))
                                 .add_message("シェーダーの作成に失敗しました");
        if (shader_result.is_err()) {
            return shader_result.propagation(core::SystemError::ConstructRenderPassError);
        }
        std::vector<types::RenderHandle> shader_refrections;
        for (auto& s : shader_result.unwrap()) {
            if (s.input_layout.is_valid()) {
                description.vertex_layout = s.input_layout;
            }
            description.shaders.emplace_back(s.shader);
            shader_refrections.emplace_back(s.shader_reflection);
        }

        // レンダーパスの生成
        const auto render_pass_result = render_pass->make_render_pass(description);
        if (render_pass_result.is_err()) {
            return render_pass_result.propagation(core::SystemError::ConstructRenderPassError);
        }

        return render_pass;
    }

    types::DependencyNode ShadowMapRenderPassConstructor::get_node(void) const noexcept {
        return NODE;
    }

    foundation::Option<types::DependencyBounds> ShadowMapRenderPassConstructor::get_dependencies(
        void) const noexcept {
        return foundation::Option<types::DependencyBounds>();
    }
} // namespace enishi