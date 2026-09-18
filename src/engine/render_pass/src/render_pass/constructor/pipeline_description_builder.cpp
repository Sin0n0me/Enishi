#include "pipeline_description_builder.h"
#include "helper.h"
#include <render_pass/render_pass.h>

namespace enishi::render_pass {
    PipelineDescriptionConstructer::PipelineDescriptionConstructer(
        platform::IRenderer* const renderer, const platform::IWindow* const window)
        : renderer(renderer)
        , window(window) {
    }

    PipelineDescriptionConstructer* PipelineDescriptionConstructer::add_topology(
        const types::PrimitiveTopology topology) noexcept {
        this->description.topology = topology;
        return this;
    }

    PipelineDescriptionConstructer* PipelineDescriptionConstructer::add_shaders(
        const platform::IShaderDataProvider* shader_data_provider,
        const std::span<const std::tuple<types::ShaderKind, std::filesystem::path>>
            shader_paths) noexcept {
        const auto result_shader_entries = shader_data_provider->get(shader_paths);
        if (result_shader_entries.is_err()) {
            return this;
        }
        auto&& shader_entries = make_shader_map_presorted(result_shader_entries.unwrap());
        const auto result_shader = make_shaders(this->renderer, std::move(shader_entries));
        if (result_shader.is_err()) {
            return this;
        }
        auto&& shaders = result_shader.unwrap();
        for (auto& shader : shaders) {
            if (shader.shader.is_valid()) {
                this->description.shaders.emplace_back(shader.shader);
            }
            if (shader.shader_reflection.is_valid()) {
                this->description.shader_reflections.emplace_back(shader.shader_reflection);
            }
            if (shader.input_layout.is_valid()) {
                this->description.vertex_layout = shader.input_layout;
            }
        }

        return this;
    }

    PipelineDescriptionConstructer* PipelineDescriptionConstructer::add_render_target_view(
        const types::RenderHandle handle) noexcept {
        if (!handle.is_valid()) {
            this->errors.push_back("Add render target view: 無効なハンドルです");
            return this;
        }

        this->description.render_target_view = handle;

        return this;
    }

    PipelineDescriptionConstructer* PipelineDescriptionConstructer::add_render_target_view(
        const types::ImageFormat format) noexcept {
        const auto opt_window_size = window->get_size();
        if (opt_window_size.is_none()) {
            this->errors.push_back("ウィンドウサイズを取得できませんでした");
            return this;
        }
        const auto window_size = opt_window_size.unwrap().to_glm_ivec2();

        auto view = make_render_target(
            types::ImageDescription::make_render_target(window_size), format, this->renderer);
        if (view.is_err()) {
            this->errors.push_back(view.unwrap_err().get_message());
            return this;
        }

        this->description.render_target_view = view.unwrap();

        return this;
    }

    PipelineDescriptionConstructer*
    PipelineDescriptionConstructer::add_swap_chain_render_target_view(
        const types::ImageFormat format) noexcept {
        const auto opt_window_size = window->get_size();
        if (opt_window_size.is_none()) {
            this->errors.push_back("ウィンドウサイズを取得できませんでした");
            return this;
        }
        const auto window_size = opt_window_size.unwrap().to_glm_ivec2();

        auto view =
            make_render_target(types::ImageDescription::make_swap_chain_render_target(window_size),
                format,
                this->renderer);
        if (view.is_err()) {
            this->errors.push_back(view.unwrap_err().get_message());
            return this;
        }

        this->description.render_target_view = view.unwrap();

        return this;
    }

    PipelineDescriptionConstructer* PipelineDescriptionConstructer::add_depth_stencil_view(
        const types::RenderHandle handle) noexcept {
        if (!handle.is_valid()) {
            this->errors.push_back("Add depth stencil view: 無効なハンドルです");
            return this;
        }

        this->description.depth_stencil_view = handle;
    }

    PipelineDescriptionConstructer* PipelineDescriptionConstructer::add_depth_stencil_view(
        const types::ImageFormat format) noexcept {
        const auto opt_window_size = window->get_size();
        if (opt_window_size.is_none()) {
            this->errors.push_back("ウィンドウサイズを取得できませんでした");
            return this;
        }
        const auto window_size = opt_window_size.unwrap().to_glm_ivec2();

        auto&& view = make_depth_stencil(
            types::ImageDescription::make_depth_stencil(window_size), format, this->renderer);
        if (view.is_err()) {
            this->errors.push_back(view.unwrap_err().get_message());
            return this;
        }

        this->description.depth_stencil_view = view.unwrap();

        return this;
    }

    PipelineDescriptionConstructer* PipelineDescriptionConstructer::add_rasterizer_state(
        const types::RenderHandle handle) noexcept {
        if (!handle.is_valid()) {
            this->errors.push_back("Add rasterizer state: 無効なハンドルです");
            return this;
        }

        this->description.rasterizer_state = handle;
    }

    PipelineDescriptionConstructer* PipelineDescriptionConstructer::add_rasterizer_state(
        types::RasterizerStateDescription&& description) noexcept {
        auto&& rasterizer = make_rasterizer(std::move(description), this->renderer);
        if (rasterizer.is_err()) {
            this->errors.push_back(rasterizer.unwrap_err().get_message());
            return this;
        }

        this->description.rasterizer_state = rasterizer.unwrap();

        return this;
    }

    foundation::Result<std::shared_ptr<platform::IRenderPass>, ConstructError>
    PipelineDescriptionConstructer::build_render_pass(
        IRenderPassConstructor* const constructor) noexcept {
        auto render_pass = std::make_shared<RenderPass>();
        const auto render_pass_result = render_pass
                                            ->make_from_description(std::move(this->description),
                                                constructor->get_render_pass_name(),
                                                constructor->get_node(),
                                                constructor->get_dependencies())
                                            .add_message(this->errors.join("\n"));
        if (render_pass_result.is_err()) {
            return render_pass_result.propagation(ConstructError::Construct);
        }

        return render_pass;
    }
} // namespace enishi::render_pass