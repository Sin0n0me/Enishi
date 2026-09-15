#pragma once
#include <foundation/constexpr/hash/char_array_to_hash.h>
#include <render_pass/constructor/interface_render_pass_constructor.h>

namespace enishi::render_pass {
    class RenderPassConstructor : public IRenderPassConstructor {
      public:
        static constexpr char RENDER_PASS_NAME[] = "";
        static constexpr types::DependencyNode NODE{foundation::hash_size_t(RENDER_PASS_NAME)};

      public:
        foundation::Result<std::shared_ptr<platform::IRenderPass>, core::SystemError> make(
            platform::IRenderer* const renderer) override;

        std::vector<std::filesystem::path> get_paths(void) const noexcept override;
        types::DependencyNode get_node(void) const noexcept override;
        foundation::Option<types::DependencyBounds> get_dependencies(void) const noexcept override;
    };
} // namespace enishi::render_pass