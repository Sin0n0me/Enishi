#include "back_ground_render_pass_constructor.h"
#include "helper.h"
#include "shadow_map_render_pass_constructor.h"
#include <foundation/log/logger.h>
#include <foundation/path/path_utility.h>
#include <foundation/str/string_builder.h>
#include <renderer/common/render_pass/render_pass.h>
#include <settings.h>

namespace enishi {
    constexpr char VS_FILE_NAME[] = "vs_clear_wall";
    constexpr char PS_FILE_NAME[] = "ps_clear_wall";
    constexpr float WALL_SIZE = 30.0f;
    constexpr float HALF_WALL_SIZE = WALL_SIZE / 2.0f;
    constexpr float FRONT_DEPTH = 0.0f;
    constexpr float FLOOR_DEPTH = 1.5f; // 奥行(床面)
    constexpr float OFFSET_Y = 0.0f;

    foundation::Result<std::shared_ptr<platform::IRenderPass>, core::SystemError>
    BackGroundRenderPassConstructor::make(
        platform::IRenderer* const renderer, assets_system::IAssetSystem* const asset_system) {
        auto render_pass = std::make_shared<renderer::RenderPass>();

        types::PipelineDescription description{
            .topology = types::PrimitiveTopology::TriangleList,
        };

        // レンダーターゲットの作成
        auto rtv =
            make_render_target(types::ImageDescription::make_default_render_target(WINDOW_SIZE),
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

        // ブレンドステートの作成
        auto blend_state = make_blend_state(
            types::BlendStateDescription{
                .alpha_to_coverage = true,
                .independent_blend = true,
                .render_targets = types::RenderTargetBlendState::default_blend_state(),
            },
            renderer);
        if (blend_state.is_err()) {
            return std::move(blend_state).unwrap_err();
        }
        description.blend_state = blend_state.unwrap();

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

        assets_system::AssetModelData model_data = std::make_shared<types::ModelData>();
        model_data->vertices = {
            types::VertexVariants{
                types::VertexPosition{
                    .position = {-HALF_WALL_SIZE, -HALF_WALL_SIZE, FRONT_DEPTH + FLOOR_DEPTH},
                },
            },
            types::VertexVariants{
                types::VertexPosition{
                    .position = {-HALF_WALL_SIZE, WALL_SIZE, FRONT_DEPTH + FLOOR_DEPTH},
                },
            },
            types::VertexVariants{
                types::VertexPosition{
                    .position = {HALF_WALL_SIZE, WALL_SIZE, FRONT_DEPTH + FLOOR_DEPTH},
                },
            },
            types::VertexVariants{
                types::VertexPosition{
                    .position = {HALF_WALL_SIZE, -HALF_WALL_SIZE, FRONT_DEPTH + FLOOR_DEPTH},
                },
            },
        };

        model_data->indices = {std::vector<std::uint16_t>{
            0,
            1,
            2,
            0,
            2,
            3,
        }};

        // メッシュ作成
        auto mesh_handle = renderer->create_mesh(*model_data, shader_refrections);
        if (mesh_handle.is_err()) {
            return mesh_handle.propagation(core::SystemError::ConstructRenderPassError);
        }
        render_pass->add_mesh("Wall", mesh_handle.unwrap());

        return render_pass;
    }

    types::DependencyNode BackGroundRenderPassConstructor::get_node(void) const noexcept {
        return NODE;
    }

    foundation::Option<types::DependencyBounds> BackGroundRenderPassConstructor::get_dependencies(
        void) const noexcept {
        return types::DependencyBounds{.precedents = {
                                           ShadowMapRenderPassConstructor::NODE,
                                       }};
    }
} // namespace enishi