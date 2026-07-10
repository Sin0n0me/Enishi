#include "render_pass_constructor.h"
#include <foundation/log/logger.h>

namespace enishi {
    const std::filesystem::path SHADER_PATH = "./assets/shader";
    constexpr char PASS_MODEL[] = "ModelRenderPass";

    foundation::UTF8 path_to_regex(const std::filesystem::path& path) {
        constexpr std::string_view kMetaChars = R"(\.^$|()[]{}*+?-)";
        foundation::UTF8 result;
        const auto str = path.lexically_normal().string<char>();
        result.reserve(str.size() * 2);

        for (const char c : str) {
            if (kMetaChars.contains(c)) {
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

    RenderPassConstructor RenderPassConstructor::make(std::weak_ptr<platform::IRenderer> renderer,
        std::weak_ptr<assets_system::IAssetSystem> asset_system) {
        RenderPassConstructor constructor(renderer, asset_system);

        constructor.find_shader();

        return constructor;
    }

    platform::RenderResult<types::RenderPass> RenderPassConstructor::make_model_render_pass(void) {
        auto renderer = this->renderer.lock();
        if (!bool(renderer)) {
            return foundation::Error(platform::RenderError::MakeError, "Renderer が存在しません");
        }
        auto asset_system = this->asset_system.lock();
        if (!bool(asset_system)) {
            return foundation::Error(
                platform::RenderError::MakeError, "Asset System が存在しません");
        }

        types::PipelineDescription& description = this->name_to_description[PASS_MODEL];

        // シェーダーからインプットレイアウトの作成
        const auto vertex_file = SHADER_PATH / "vs_model";
        const auto extension_pattern = asset_system->shader_extensions_pattern();
        const auto str_pattern = std::format("{}{}", path_to_regex(vertex_file), extension_pattern);
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
            const auto handle = this->make_shader(pattern, renderer, asset_system);
            if (handle.is_err()) {
                return handle.propagation(
                    platform::RenderError::MakeError, "頂点シェーダーの作成に失敗しました");
            }

            description.shaders.push_back(handle.value());
        }

        {
            const auto pixel_file = SHADER_PATH / "ps_model";
            const std::regex pattern(
                std::format("{}{}", path_to_regex(pixel_file), extension_pattern));

            const auto handle = this->make_shader(pattern, renderer, asset_system);
            if (handle.is_err()) {
                return handle.propagation(
                    platform::RenderError::MakeError, "ピクセルシェーダーの作成に失敗しました");
            }

            description.shaders.push_back(handle.value());
        }

        return renderer->create_render_pass(description);
    }

    platform::RenderResult<void> RenderPassConstructor::find_shader(void) {
        auto asset_system = this->asset_system.lock();
        if (!bool(asset_system)) {
            return foundation::Error(
                platform::RenderError::MakeError, "Asset Systemが存在しません");
        }

        this->shader_paths = asset_system->find_shaders(SHADER_PATH);

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

            const auto shader_handle = renderer->create_shader(shader_data.value());
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
} // namespace enishi