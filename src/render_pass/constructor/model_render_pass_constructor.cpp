#include "model_render_pass_constructor.h"
#include "../../settings.h"
#include <foundation/log/logger.h>
#include <foundation/path/path_utility.h>
#include <renderer/common/render_pass.h>"

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

        // RTVの作成
        auto rtv = this->make_render_target(description, renderer);
        if (rtv.is_err()) {
            return std::move(rtv).unwrap_err();
        }
        // ラスタライザの作成
        auto rasterizer = this->make_rasterizer(description, renderer);
        if (rasterizer.is_err()) {
            return std::move(rasterizer).unwrap_err();
        }

        // シェーダーの作成
        auto shaders = this->make_shaders(description, renderer, asset_system);

        // レンダーパスの生成
        const auto render_pass_result = render_pass->make_render_pass(description);
        if (render_pass_result.is_err()) {
            return render_pass_result.propagation(core::SystemError::ConstructRenderPassError);
        }

        // モデルのみ初期モデル追加
        // TODO: ファイルからの読み取り初期モデルを選択するように
        const auto mesh_result = this->make_mesh(renderer, asset_system)
                                     .add_message("メッシュデータの作成に失敗しました");
        if (mesh_result.is_err()) {
            return mesh_result.propagation(core::SystemError::ConstructRenderPassError);
        }
        render_pass->add_mesh(mesh_result.unwrap(), {});

        return render_pass;
    }

    foundation::VoidResult<core::SystemError> ModelRenderPassConstructor::make_render_target(
        types::PipelineDescription& description, platform::IRenderer* const renderer) {
        const auto image_description =
            types::ImageDescription::make_default_render_target(WINDOW_SIZE);
        const auto image_handle =
            renderer->create_image(image_description).add_message("イメージの作成に失敗しました");
        if (image_handle.is_err()) {
            return image_handle.propagation(core::SystemError::ConstructRenderPassError);
        }

        const auto image_view_description = types::ImageViewDescription{};
        const auto result =
            renderer->create_render_target_view(image_handle.unwrap(), image_view_description)
                .add_message("レンダーターゲットの作成に失敗しました");

        if (result.is_err()) {
            return result.propagation(core::SystemError::ConstructRenderPassError);
        }
        auto render_target_view = result.unwrap().lock();
        if (!bool(render_target_view)) {
            return foundation::Error(core::SystemError::ConstructRenderPassError);
        }

        render_target_view->set_clear_color(CLEAR_COLOR);

        description.render_target = render_target_view->get_handle();

        return {};
    }

    foundation::VoidResult<core::SystemError> ModelRenderPassConstructor::make_rasterizer(
        types::PipelineDescription& description, platform::IRenderer* const renderer) {
        const auto rasterizer = renderer
                                    ->create_rasterizer(types::RasterizerDescription{
                                        .cull_mode = types::CullMode::None,
                                        .front_face = types::FrontFace::CounterClockwise,
                                    })
                                    .add_message("ラスタライザの作成に失敗しました");

        if (rasterizer.is_err()) {
            return rasterizer.propagation(core::SystemError::ConstructRenderPassError);
        }

        description.rasterizer = rasterizer.unwrap();

        return {};
    }

    foundation::VoidResult<core::SystemError> ModelRenderPassConstructor::make_input_layout(
        types::PipelineDescription& description,
        platform::IRenderer* const renderer,
        assets_system::IAssetSystem* const asset_system,
        const std::vector<std::filesystem::path>& asset_paths) {
        const bool multiple_shaders_found = 1 < asset_paths.size();
        if (multiple_shaders_found) {
            foundation::Logger::warning("複数のシェーダーファイルが見つかりました");
        }

        // シェーダーの作成
        // 複数のシェーダーが見つかった場合は最初に正常に作成できたシェーダーを使用
        foundation::UTF8 error_message;
        for (const auto& path : asset_paths) {
            const auto asset_handle = asset_system->load_asset(path);
            if (asset_handle.is_err()) {
                error_message += asset_handle.unwrap_err().get_message() + "\n";
                error_message += std::format("読み込みに失敗しました: {}", path.string<char>());
                continue;
            }
            const auto shader_data = asset_system->get_shader_data(asset_handle.unwrap());
            if (shader_data.is_none()) {
                error_message +=
                    std::format("シェーダーデータの取得に失敗しました: {}", path.string<char>());
                continue;
            }

            // シェーダーリフレクションで情報の取得
            const auto shader_reflection = renderer->create_shader_reflection(shader_data.unwrap());
            if (shader_reflection.is_err()) {
                error_message += shader_reflection.unwrap_err().get_message() + "\n";
                continue;
            }

            const auto input_layout =
                renderer->create_pipeline_layout_from_shader_reflection(shader_reflection.unwrap());
            if (input_layout.is_err()) {
                error_message += input_layout.unwrap_err().get_message() + "\n";
                error_message +=
                    std::format("頂点レイアウトの作成に失敗しました: {}", path.string<char>());
                continue;
            }

            description.vertex_layout = input_layout.unwrap();
            return {};
        }

        return foundation::Error(core::SystemError::ConstructRenderPassError, error_message);
    }

    foundation::VoidResult<core::SystemError> ModelRenderPassConstructor::make_shaders(
        types::PipelineDescription& description,
        platform::IRenderer* const renderer,
        assets_system::IAssetSystem* const asset_system) {
        const auto shader_paths = asset_system->find_shaders(SHADER_PATH);
        const auto pattern_shader_extensions = asset_system->shader_extensions_pattern();

        {
            const auto vertex_file = SHADER_PATH / VS_FILE_NAME;
            const auto str_pattern = std::format(
                "{}{}", foundation::path_to_regex_str(vertex_file), pattern_shader_extensions);
            const std::regex pattern(str_pattern);
            const auto paths = shader_paths.find(pattern);

            const auto handle =
                this->make_shader(types::ShaderKind::Vertex, paths, renderer, asset_system)
                    .add_message("頂点シェーダーの作成に失敗しました");
            if (handle.is_err()) {
                return handle.propagation(core::SystemError::ConstructRenderPassError);
            }

            description.shaders.emplace_back(handle.unwrap());

            // シェーダーからインプットレイアウトの作成
            auto&& input_layout =
                this->make_input_layout(description, renderer, asset_system, paths);
            if (input_layout.is_err()) {
                return input_layout;
            }
        }

        {
            const auto pixel_file = SHADER_PATH / PS_FILE_NAME;
            const auto str_pattern = std::format(
                "{}{}", foundation::path_to_regex_str(pixel_file), pattern_shader_extensions);
            const std::regex pattern(str_pattern);

            const auto handle = this->make_shader(types::ShaderKind::Pixel,
                                        shader_paths.find(pattern),
                                        renderer,
                                        asset_system)
                                    .add_message("ピクセルシェーダーの作成に失敗しました");
            if (handle.is_err()) {
                return handle.propagation(core::SystemError::ConstructRenderPassError);
            }

            description.shaders.emplace_back(handle.unwrap());
        }

        return {};
    }

    foundation::Result<types::RenderHandle, core::SystemError>
    ModelRenderPassConstructor::make_shader(const types::ShaderKind kind,
        const std::vector<std::filesystem::path>& asset_paths,
        platform::IRenderer* const renderer,
        assets_system::IAssetSystem* const asset_system) {
        if (asset_paths.empty()) {
            return foundation::Error(
                core::SystemError::ConstructRenderPassError, "シェーダーファイルが見つかりません");
        }

        const bool multiple_shaders_found = 1 < asset_paths.size();
        if (multiple_shaders_found) {
            foundation::Logger::warning("複数のシェーダーファイルが見つかりました");
        }

        // シェーダーの作成
        // 複数のシェーダーが見つかった場合は最初に正常に作成できたシェーダーを使用
        foundation::UTF8 error_message;
        for (const auto& path : asset_paths) {
            const auto asset_handle = asset_system->load_asset(path);
            if (asset_handle.is_err()) {
                error_message = asset_handle.unwrap_err().get_message();
                continue;
            }
            const auto shader_data = asset_system->get_shader_data(asset_handle.unwrap());
            if (shader_data.is_none()) {
                continue;
            }

            const auto shader_handle = renderer->create_shader(kind, *shader_data.unwrap());
            if (shader_handle.is_err()) {
                error_message = shader_handle.unwrap_err().get_message();
                continue;
            }

            if (multiple_shaders_found) {
                foundation::Logger::info(
                    std::format("使用するシェーダー: {}", path.string<char>()));
            }

            return shader_handle.unwrap();
        }

        return foundation::Error(core::SystemError::ConstructRenderPassError, error_message);
    }

    foundation::Result<types::RenderHandle, core::SystemError>
    ModelRenderPassConstructor::make_mesh(
        platform::IRenderer* const renderer, assets_system::IAssetSystem* const asset_system) {
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
        foundation::UTF8 error_message;
        error_message.reserve(0x1000);
        for (const auto& path : asset_paths) {
            error_message += std::format("loaded path: {}\n", path.string<char>());
            const auto asset_handle = asset_system->load_asset(path);
            if (asset_handle.is_err()) {
                error_message += asset_handle.unwrap_err().get_message() + "\n";
                continue;
            }
            const auto opt_model_data = asset_system->get_model_data(asset_handle.unwrap());
            if (opt_model_data.is_none()) {
                continue;
            }
            const auto& model_data = opt_model_data.unwrap();

            // 先にテクスチャ読み込み
            for (const auto& material : model_data->materials) {
                for (const auto& texture_path : material.texture_paths) {
                    const auto asset_handle = asset_system->load_asset(texture_path);
                    if (asset_handle.is_err()) {
                        error_message += asset_handle.unwrap_err().get_message() + "\n";
                    }
                }
            }

            // メッシュ作成
            const auto mesh_handle = renderer->create_mesh(model_data->to_mesh_data());
            if (mesh_handle.is_err()) {
                error_message += mesh_handle.unwrap_err().get_message() + "\n";
                continue;
            }

            return mesh_handle.unwrap();
        }

        return foundation::Error(core::SystemError::ConstructRenderPassError, error_message);
    }
} // namespace enishi