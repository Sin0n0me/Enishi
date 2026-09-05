#pragma once
#include <core/system/render/interface_render_pass_construstor.h>
#include <foundation/constexpr/hash/char_array_to_hash.h>
#include <vector>

namespace enishi {
    class ShadowMapRenderPassConstructor : public core::IRenderPassConstructor {
      public:
        static constexpr char RENDER_PASS_NAME[] = "ShadowMap";
        static constexpr types::DependencyNode NODE{foundation::hash_size_t(RENDER_PASS_NAME)};

      public:
        foundation::Result<std::shared_ptr<platform::IRenderPass>, core::SystemError> make(
            platform::IRenderer* const renderer,
            assets_system::IAssetSystem* const asset_system) override;

      private:
        types::DependencyNode get_node(void) const noexcept override;
        foundation::Option<types::DependencyBounds> get_dependencies(void) const noexcept override;
    };
} // namespace enishi