#include "render_pass_constructor.h"
#include "settings.h"
#include <foundation/log/logger.h>

namespace enishi {
    const std::filesystem::path SHADER_PATH = "./assets/shader";
    const std::filesystem::path MODEL_PATH = "./assets/models";
    constexpr char PASS_MODEL[] = "ModelRenderPass";

    foundation::UTF8 path_to_regex(const std::filesystem::path& path) {
        constexpr std::string_view META_CHARS = R"(\.^$|()[]{}*+?-)";
        foundation::UTF8 result;
        const auto str = path.lexically_normal().string<char>();
        result.reserve(str.size() * 2);

        for (const char c : str) {
            if (META_CHARS.contains(c)) {
                result += '\\';
            }
            result += c;
        }

        return result;
    }

    RenderPassConstructor::RenderPassConstructor(std::weak_ptr<platform::IRenderer> renderer,
        std::weak_ptr<assets_system::IAssetSystem> asset_system)
        : renderer(renderer)
        , asset_system(asset_system) {
    }

    foundation::Result<RenderPassConstructor, void> RenderPassConstructor::make(
        std::weak_ptr<platform::IRenderer> renderer,
        std::weak_ptr<assets_system::IAssetSystem> asset_system) {
        RenderPassConstructor constructor(renderer, asset_system);

        auto result = constructor.find_shaders();
        if (result.is_err()) {
            foundation::Logger::error(result.error().get_message());
        }
        result = constructor.find_models();
        if (result.is_err()) {
            foundation::Logger::error(result.error().get_message());
        }

        return constructor;
    }

    platform::RenderResult<types::RenderPass> RenderPassConstructor::make_model_render_pass(void) {
        auto renderer = this->renderer.lock();
        if (!bool(renderer)) {
            // foundation::Error();
            return foundation::Error(platform::RenderError::MakeError, "Renderer が存在しません");
        }
        auto asset_system = this->asset_system.lock();
        if (!bool(asset_system)) {
            return foundation::Error(
                platform::RenderError::MakeError, "Asset System が存在しません");
        }

        types::PipelineDescription& description = this->name_to_description[PASS_MODEL];

        description.topology = types::PrimitiveTopology::TriangleList;

        // RTVの作成
        {
            const auto image_description =
                types::ImageDescription::make_default_render_target(WINDOW_SIZE);
            const auto image_handle = renderer->create_image(image_description);
            if (image_handle.is_err()) {
                return image_handle.propagation(
                    platform::RenderError::MakeError, "イメージの作成に失敗しました");
            }

            const auto image_view_description = types::ImageViewDescription{};
            const auto result =
                renderer->create_render_target_view(image_handle.value(), image_view_description);

            if (result.is_err()) {
                return image_handle.propagation(
                    platform::RenderError::MakeError, "レンダーターゲットの作成に失敗しました");
            }
            if (auto render_target_view = result.value().lock()) {
                render_target_view->set_clear_color(CLEAR_COLOR);
                description.render_target = render_target_view->get_handle();
            }
        }

        // ラスタライザの作成
        {
            const auto rasterizer = renderer->create_rasterizer(types::RasterizerDescription{
                .cull_mode = types::CullMode::None,
                .front_face = types::FrontFace::CounterClockwise,
            });
            if (rasterizer.is_err()) {
                return rasterizer.propagation(
                    platform::RenderError::MakeError, "ラスタライザの作成に失敗しました");
            }

            description.rasterizer = rasterizer.value();
        }

        // シェーダーからインプットレイアウトの作成
        const auto vertex_file = SHADER_PATH / "vs_model";
        const auto pattern_shader_extensions = asset_system->shader_extensions_pattern();
        const auto str_pattern =
            std::format("{}{}", path_to_regex(vertex_file), pattern_shader_extensions);
        const std::regex pattern(str_pattern);
        {
            const auto input_layout =
                this->make_input_layout_from_shader(pattern, renderer, asset_system);
            if (input_layout.is_err()) {
                return input_layout.propagation(
                    platform::RenderError::MakeError, "入力レイアウトの作成に失敗しました");
            }

            description.vertex_layout = input_layout.value();
        }

        // シェーダーの作成
        {
            const auto handle =
                this->make_shader(types::ShaderKind::Vertex, pattern, renderer, asset_system);
            if (handle.is_err()) {
                return handle.propagation(
                    platform::RenderError::MakeError, "頂点シェーダーの作成に失敗しました");
            }

            description.shaders.push_back(handle.value());
        }

        {
            const auto pixel_file = SHADER_PATH / "ps_model";
            const std::regex pattern(
                std::format("{}{}", path_to_regex(pixel_file), pattern_shader_extensions));

            const auto handle =
                this->make_shader(types::ShaderKind::Pixel, pattern, renderer, asset_system);
            if (handle.is_err()) {
                return handle.propagation(
                    platform::RenderError::MakeError, "ピクセルシェーダーの作成に失敗しました");
            }

            description.shaders.push_back(handle.value());
        }

        auto result = renderer->create_render_pass(description);
        if (result.is_err()) {
            return result;
        }
        auto&& render_pass = result.value();

        // モデルのみ初期モデル追加
        // TODO: ファイルからの読み取り初期モデルを選択するように
        {
            const auto pattern_model_extensions = asset_system->model_extensions_pattern();
            const auto path = MODEL_PATH / "";
            const std::regex pattern(
                std::format("{}.*{}", path_to_regex(path), pattern_model_extensions));

            const auto handle = this->make_mesh(pattern, renderer, asset_system);
            if (handle.is_err()) {
                return handle.propagation(
                    platform::RenderError::MakeError, "モデルの作成に失敗しました");
            }

            render_pass.commands.push_back(types::DrawCommand{
                .handle = handle.value(),
                .sub_command = types::SubCommand::Bind,
            });
        }

        return result;
    }

    platform::RenderResult<void> RenderPassConstructor::find_shaders(void) {
        auto asset_system = this->asset_system.lock();
        if (!bool(asset_system)) {
            return foundation::Error(
                platform::RenderError::MakeError, "Asset Systemが存在しません");
        }

        this->shader_paths = asset_system->find_shaders(SHADER_PATH);

        return {};
    }

    platform::RenderResult<void> RenderPassConstructor::find_models(void) {
        auto asset_system = this->asset_system.lock();
        if (!bool(asset_system)) {
            return foundation::Error(
                platform::RenderError::MakeError, "Asset Systemが存在しません");
        }

        this->model_paths = asset_system->find_models(MODEL_PATH);

        return {};
    }

    platform::RenderResult<types::RenderHandle>
    RenderPassConstructor::make_input_layout_from_shader(const std::regex& pattern,
        const std::shared_ptr<platform::IRenderer>& renderer,
        const std::shared_ptr<assets_system::IAssetSystem>& asset_system) {
        const auto asset_paths = this->shader_paths.find(pattern);
        if (asset_paths.empty()) {
            return foundation::Error(
                platform::RenderError::MakeError, "シェーダーファイルが見つかりません");
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
                error_message += asset_handle.error().get_message() + "\n";
                error_message += std::format("読み込みに失敗しました: {}", path.string<char>());
                continue;
            }
            const auto shader_data = asset_system->get_shader_data(asset_handle.value());
            if (shader_data.is_none()) {
                error_message +=
                    std::format("シェーダーデータの取得に失敗しました: {}", path.string<char>());
                continue;
            }
            const auto input_layout =
                renderer->create_pipeline_layout_from_shader(shader_data.unwrap());
            if (input_layout.is_err()) {
                error_message += input_layout.error().get_message() + "\n";
                error_message +=
                    std::format("頂点レイアウトの作成に失敗しました: {}", path.string<char>());
                continue;
            }

            return input_layout.value();
        }

        return foundation::Error(platform::RenderError::MakeError, error_message);
    }

    platform::RenderResult<types::RenderHandle> RenderPassConstructor::make_shader(
        const types::ShaderKind kind,
        const std::regex& pattern,
        const std::shared_ptr<platform::IRenderer>& renderer,
        const std::shared_ptr<assets_system::IAssetSystem>& asset_system) {
        const auto asset_paths = this->shader_paths.find(pattern);
        if (asset_paths.empty()) {
            return foundation::Error(
                platform::RenderError::MakeError, "シェーダーファイルが見つかりません");
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
                error_message = asset_handle.error().get_message();
                continue;
            }
            const auto shader_data = asset_system->get_shader_data(asset_handle.value());
            if (shader_data.is_none()) {
                continue;
            }

            const auto shader_handle = renderer->create_shader(kind, shader_data.value());
            if (shader_handle.is_err()) {
                error_message = shader_handle.error().get_message();
                continue;
            }

            if (multiple_shaders_found) {
                foundation::Logger::info(
                    std::format("使用するシェーダー: {}", path.string<char>()));
            }

            return shader_handle.value();
        }

        return foundation::Error(platform::RenderError::MakeError, error_message);
    }

    platform::RenderResult<types::RenderHandle> RenderPassConstructor::make_mesh(
        const std::regex& pattern,
        const std::shared_ptr<platform::IRenderer>& renderer,
        const std::shared_ptr<assets_system::IAssetSystem>& asset_system) {
        const auto asset_paths = this->model_paths.find(pattern);

        if (asset_paths.empty()) {
            return foundation::Error(
                platform::RenderError::MakeError, "モデルデータが見つかりません");
        }

        // モデルからメッシュへ変換
        foundation::UTF8 error_message;
        for (const auto& path : asset_paths) {
            const auto asset_handle = asset_system->load_asset(path);
            if (asset_handle.is_err()) {
                error_message = asset_handle.error().get_message();
                continue;
            }
            const auto opt_model_data = asset_system->get_model_data(asset_handle.value());
            if (opt_model_data.is_none()) {
                continue;
            }
            const auto& model_data = opt_model_data.unwrap();

            const auto mesh_handle = renderer->create_mesh(model_data.to_mesh_data());
            if (mesh_handle.is_err()) {
                error_message = mesh_handle.error().get_message();
                continue;
            }

            return mesh_handle.value();
        }

        return foundation::Error(platform::RenderError::MakeError, error_message);
    }
} // namespace enishi