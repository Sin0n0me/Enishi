#include "model_render_pass_constructor.h"
#include "back_ground_render_pass_constructor.h"
#include "shadow_map_render_pass_constructor.h"
#include <foundation/log/logger.h>
#include <foundation/path/path_utility.h>
#include <foundation/str/string_builder.h>
#include <renderer/common/render_pass/render_pass.h>
#include <settings.h>

namespace enishi {
    const std::filesystem::path SHADER_PATH = "./assets/shader";
    const std::filesystem::path MODEL_PATH = "./assets/models";
    constexpr char VS_FILE_NAME[] = "vs_model";
    constexpr char PS_FILE_NAME[] = "ps_model";

    foundation::Result<std::shared_ptr<platform::IRenderPass>, core::SystemError>
    ModelRenderPassConstructor::make(
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

        // モデルのみ初期モデル追加
        // TODO: ファイルからの読み取り初期モデルを選択するように
        const auto mesh_result = this->make_mesh(renderer, asset_system, shader_refrections)
                                     .add_message("メッシュデータの作成に失敗しました");
        if (mesh_result.is_err()) {
            return mesh_result.propagation(core::SystemError::ConstructRenderPassError);
        }
        auto& [name, handle] = mesh_result.unwrap();
        render_pass->add_mesh(name, handle);

        return render_pass;
    }

    foundation::Result<std::tuple<foundation::UTF8, types::RenderHandle>, core::SystemError>
    ModelRenderPassConstructor::make_mesh(platform::IRenderer* const renderer,
        assets_system::IAssetSystem* const asset_system,
        const std::vector<types::RenderHandle>& shader_reflections) {
        const auto pattern_model_extensions = asset_system->model_extensions_pattern();
        const auto path = MODEL_PATH / "";
        const std::regex pattern(
            std::format("{}.*{}", foundation::path_to_regex_str(path), pattern_model_extensions));
        const auto model_paths = asset_system->find_models(MODEL_PATH);
        const auto asset_paths = model_paths.find(pattern);

        if (asset_paths.empty()) {
            return foundation::Error(
                core::SystemError::ConstructRenderPassError, "モデルデータが見つかりません");
        }

        // モデルからメッシュへ変換
        foundation::StringBuilder error_message;
        for (const auto& path : asset_paths) {
            error_message.push_back(std::format("loaded path: {}", path.string<char>()));
            const auto asset_handle = asset_system->load_asset(path);
            if (asset_handle.is_err()) {
                error_message.push_back(asset_handle.unwrap_err().get_message());
                continue;
            }
            const auto opt_model_data = asset_system->get_model_data(asset_handle.unwrap());
            if (opt_model_data.is_none()) {
                continue;
            }
            const auto& model_data = opt_model_data.unwrap();

            // 先にテクスチャ読み込み
            for (const auto& material : model_data->materials) {
                for (const auto& material_texture : material.textures) {
                    const auto asset_handle = asset_system->load_asset(material_texture.path);
                    if (asset_handle.is_err()) {
                        error_message.push_back(asset_handle.unwrap_err().get_message());
                    }
                }
            }

            // メッシュ作成
            const auto mesh_handle = renderer->create_mesh(*model_data, shader_reflections);
            if (mesh_handle.is_err()) {
                error_message.push_back(mesh_handle.unwrap_err().get_message());
                continue;
            }

            return std::tuple{
                model_data->name,
                mesh_handle.unwrap(),
            };
        }

        return foundation::Error(
            core::SystemError::ConstructRenderPassError, error_message.join("\n"));
    }

    types::DependencyNode ModelRenderPassConstructor::get_node(void) const noexcept {
        return NODE;
    }

    foundation::Option<types::DependencyBounds> ModelRenderPassConstructor::get_dependencies(
        void) const noexcept {
        return types::DependencyBounds{.precedents = {
                                           ShadowMapRenderPassConstructor::NODE,
                                           BackGroundRenderPassConstructor::NODE,
                                       }};
    }
} // namespace enishi