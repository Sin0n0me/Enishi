#pragma once
#include <functional>
#include <platform/renderer/updater/interface_resource_updater.h>

namespace enishi::renderer {
    class VertexBufferUpdater final : public platform::IResourceUpdater {
      private:
        types::OwnedRenderData resource;
        std::function<void(const types::RenderData&)> upload;

      public:
        VertexBufferUpdater(types::OwnedRenderData&& resource,
            std::function<void(const types::RenderData&)> upload);
        void on_update(void) override;
        types::OwnedRenderData& get_resource(void) override;
    };
} // namespace enishi::renderer
