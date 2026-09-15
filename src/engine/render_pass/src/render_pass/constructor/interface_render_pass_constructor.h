#pragma once
#include <filesystem>
#include <foundation/algorithm/dependency_bounds.h>
#include <foundation/result/result.h>
#include <foundation/str/str.h>
#include <memory>
#include <platform/asset/interface_shader_data_provider.h>
#include <platform/renderer/interface_render_pass.h>
#include <platform/renderer/interface_renderer.h>
#include <platform/window/interface_window.h>
#include <render_pass/errors/errors.h>
#include <tuple>
#include <vector>

namespace enishi::render_pass {
    class IRenderPassConstructor {
      public:
        virtual ~IRenderPassConstructor(void) noexcept = default;

        [[nodiscard]] virtual foundation::Result<std::shared_ptr<platform::IRenderPass>,
            ConstructError>
        make(platform::IRenderer* const renderer,
            const platform::IWindow* window,
            const platform::IShaderDataProvider* shader_data_provider) = 0;

        // 拡張子は含めないこと
        [[nodiscard]] virtual std::vector<std::tuple<types::ShaderKind, std::filesystem::path>>
        get_paths(void) const noexcept = 0;

        [[nodiscard]] virtual foundation::UTF8 get_render_pass_name(void) const noexcept = 0;

        [[nodiscard]] virtual foundation::DependencyNode get_node(void) const noexcept = 0;

        [[nodiscard]] virtual foundation::DependencyBounds get_dependencies(
            void) const noexcept = 0;
    };
} // namespace enishi::render_pass