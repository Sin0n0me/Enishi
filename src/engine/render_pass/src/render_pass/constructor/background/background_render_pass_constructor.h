#pragma once
#include <foundation/constexpr/hash/char_array_to_hash.h>
#include <render_pass/constructor/interface_render_pass_constructor.h>
#include <vector>

namespace enishi::render_pass {
    class BackgroundRenderPassConstructor : public IRenderPassConstructor {
      public:
        static constexpr char RENDER_PASS_NAME[] = "Background";
        static constexpr foundation::DependencyNode NODE{foundation::hash_size_t(RENDER_PASS_NAME)};

      public:
        foundation::Result<std::shared_ptr<platform::IRenderPass>, ConstructError> make(
            platform::IRenderer* const renderer,
            const platform::IWindow* window,
            const platform::IShaderDataProvider* shader_data_provider) override;
        std::vector<std::tuple<types::ShaderKind, std::filesystem::path>> get_paths(
            void) const noexcept override;
        foundation::UTF8 get_render_pass_name(void) const noexcept override;
        foundation::DependencyNode get_node(void) const noexcept override;
        foundation::DependencyBounds get_dependencies(void) const noexcept override;
    };
} // namespace enishi::render_pass