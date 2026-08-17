#include "model_render_pass_constructor.h"
#include "../../settings.h"
#include <foundation/log/logger.h>
#include <foundation/path/path_utility.h>
#include <foundation/str/string_builder.h>
#include <renderer/common/render_pass.h>

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
        auto shader_reflections = this->make_shaders(description, renderer, asset_system)
                                      .add_message("シェーダーの作成に失敗しました");
        if (shader_reflections.is_err()) {
            return shader_reflections.propagation(core::SystemError::ConstructRenderPassError);
        }

        // レンダーパスの生成
        const auto render_pass_result = render_pass->make_render_pass(description);
        if (render_pass_result.is_err()) {
            return render_pass_result.propagation(core::SystemError::ConstructRenderPassError);
        }

        // モデルのみ初期モデル追加
        // TODO: ファイルからの読み取り初期モデルを選択するように
        const auto mesh_result =
            this->make_mesh(renderer, asset_system, shader_reflections.unwrap())
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

    foundation::Result<std::vector<types::RenderHandle>, core::SystemError>
    ModelRenderPassConstructor::make_shaders(types::PipelineDescription& description,
        platform::IRenderer* const renderer,
        assets_system::IAssetSystem* const asset_system) {
        const auto shader_paths = asset_system->find_shaders(SHADER_PATH);
        const auto pattern_shader_extensions = asset_system->shader_extensions_pattern();
        const auto make_paths = [&](const std::filesystem::path& file_path) {
            const auto str_pattern = std::format(
                "{}{}", foundation::path_to_regex_str(file_path), pattern_shader_extensions);
            const std::regex pattern(str_pattern);
            return shader_paths.find(pattern);
        };

        auto shader_reflections = std::vector<types::RenderHandle>();

        {
            const auto vertex_file = SHADER_PATH / VS_FILE_NAME;
            const auto paths = make_paths(vertex_file);
            auto result = this->make_shader_from_file_paths(
                                  types::ShaderKind::Vertex, paths, renderer, asset_system)
                              .add_message("頂点シェーダーの作成に失敗しました");
            if (result.is_err()) {
                return std::move(result).unwrap_err();
            }
            const auto& handles = result.unwrap();

            description.shaders.emplace_back(handles.shader);
            description.vertex_layout = handles.input_layout;
            shader_reflections.emplace_back(handles.shader_reflection);
        }

        {
            const auto pixel_file = SHADER_PATH / PS_FILE_NAME;
            const auto paths = make_paths(pixel_file);
            const auto result = this->make_shader_from_file_paths(
                                        types::ShaderKind::Pixel, paths, renderer, asset_system)
                                    .add_message("ピクセルシェーダーの作成に失敗しました");
            if (result.is_err()) {
                return result.propagation(core::SystemError::ConstructRenderPassError);
            }
            const auto& handles = result.unwrap();

            description.shaders.emplace_back(handles.shader);
            shader_reflections.emplace_back(handles.shader_reflection);
        }

        return shader_reflections;
    }

    foundation::Result<ModelRenderPassConstructor::ShaderResult, core::SystemError>
    ModelRenderPassConstructor::make_shader_from_file_paths(const types::ShaderKind kind,
        const std::vector<std::filesystem::path>& paths,
        platform::IRenderer* const renderer,
        assets_system::IAssetSystem* const asset_system) {
        const bool multiple_shaders_found = 1 < paths.size();
        if (multiple_shaders_found) {
            foundation::Logger::warning("複数のシェーダーファイルが見つかりました");
        }

        // シェーダーの作成
        // 複数のシェーダーが見つかった場合は最初に正常に作成できたシェーダーを使用
        foundation::StringBuilder error_messages;
        for (const auto& path : paths) {
            auto result = this->make_shader(kind, path, renderer, asset_system);
            if (result.is_err()) {
                error_messages.push_back(result.unwrap_err().get_message());
                continue;
            }

            return result;
        }

        return foundation::Error(
            core::SystemError::ConstructRenderPassError, error_messages.join("\n"));
    }

    foundation::Result<ModelRenderPassConstructor::ShaderResult, core::SystemError>
    ModelRenderPassConstructor::make_shader(const types::ShaderKind kind,
        const std::filesystem::path& path,
        platform::IRenderer* const renderer,
        assets_system::IAssetSystem* const asset_system) {
        // ファイル読み込み
        const auto asset_handle = asset_system->load_asset(path);
        if (asset_handle.is_err()) {
            return asset_handle.propagation(core::SystemError::ConstructRenderPassError);
        }
        const auto shader_data = asset_system->get_shader_data(asset_handle.unwrap());
        if (shader_data.is_none()) {
            return asset_handle.propagation(core::SystemError::ConstructRenderPassError);
        }

        // シェーダーの作成
        const auto& raw_shader_data = *shader_data.unwrap();
        const auto shader = renderer->create_shader(kind, raw_shader_data);
        if (shader.is_err()) {
            return asset_handle.propagation(core::SystemError::ConstructRenderPassError);
        }

        // シェーダーリフレクションの作成(こちらは最悪失敗してもよい)
        const auto& shader_reflection = renderer->create_shader_reflection(raw_shader_data);

        // 頂点のシェーダーの場合はシェーダーデータからリフレクション作成
        auto input_layout = types::INVALID_RENDER_HANDLE;
        if (kind == types::ShaderKind::Vertex) {
            const auto result_input_layout =
                renderer->create_vertex_layout_from_shader_data(raw_shader_data)
                    .add_message("頂点レイアウトの作成に失敗しました");
            if (result_input_layout.is_err()) {
                return result_input_layout.propagation(core::SystemError::ConstructRenderPassError);
            }
            input_layout = result_input_layout.unwrap();
        }

        return ShaderResult{
            .shader = shader.unwrap(),
            .shader_reflection = shader_reflection.unwrap_or(types::INVALID_RENDER_HANDLE),
            .input_layout = input_layout,
        };
    }

    foundation::Result<types::RenderHandle, core::SystemError>
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
                for (const auto& texture_path : material.texture_paths) {
                    const auto asset_handle = asset_system->load_asset(texture_path);
                    if (asset_handle.is_err()) {
                        error_message.push_back(asset_handle.unwrap_err().get_message());
                    }
                }
            }

            // メッシュ作成
            const auto mesh_handle =
                renderer->create_mesh(model_data->to_mesh_data(), shader_reflections);
            if (mesh_handle.is_err()) {
                error_message.push_back(mesh_handle.unwrap_err().get_message());
                continue;
            }

            return mesh_handle.unwrap();
        }

        return foundation::Error(
            core::SystemError::ConstructRenderPassError, error_message.join("\n"));
    }
} // namespace enishi