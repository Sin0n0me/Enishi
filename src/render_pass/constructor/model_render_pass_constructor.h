#pragma once
#include "helper.h"
#include <core/system/render/interface_render_pass_construstor.h>
#include <foundation/constexpr/hash/char_array_to_hash.h>
#include <vector>

namespace enishi {
    class ModelRenderPassConstructor : public core::IRenderPassConstructor {
      public:
        static constexpr char RENDER_PASS_NAME[] = "Model";
        static constexpr types::DependencyNode NODE{foundation::hash_size_t(RENDER_PASS_NAME)};

      public:
        foundation::Result<std::shared_ptr<platform::IRenderPass>, core::SystemError> make(
            platform::IRenderer* const renderer,
            assets_system::IAssetSystem* const asset_system) override;

      private:
        foundation::Result<std::tuple<foundation::UTF8, types::RenderHandle>, core::SystemError>
        make_mesh(platform::IRenderer* const renderer,
            assets_system::IAssetSystem* const asset_system,
            const std::vector<types::RenderHandle>& shader_reflections);
        types::DependencyNode get_node(void) const noexcept override;
        foundation::Option<types::DependencyBounds> get_dependencies(void) const noexcept override;
    };
} // namespace enishi