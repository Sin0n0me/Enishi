#pragma once
#include "../../../common/interface_gpu_resource_accessor.h"
#include <foundation/option/option.h>
#include <memory>
#include <platform/renderer/interface_image_view.h>
#include <vector>

namespace enishi::renderer::directx {
    class ResourceEditor : public GPUResourceAccessor {
      private:
        enum class ResourceType {
            RenderTarget,
        };

        struct Resource {
            ResourceType type;
            std::size_t index;
        };

      private:
        std::unordered_map<types::HandleId, Resource> handle_to_index;
        std::vector<std::shared_ptr<platform::IRenderTargetView>> render_targets;

      public:
        void add_render_target(std::shared_ptr<platform::IRenderTargetView> rtv);

      public:
        foundation::Option<std::shared_ptr<platform::IRenderTargetView>> get_render_target(
            const types::HandleId handle) const override;
        const std::vector<std::shared_ptr<platform::IRenderTargetView>>& get_render_targets(
            void) const override;
    };
} // namespace enishi::renderer::directx