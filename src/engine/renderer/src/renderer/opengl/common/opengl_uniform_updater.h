#pragma once

#include <platform/renderer/updater/interface_uniform_updater.h>

namespace enishi::renderer::opengl {
    class OpenGLUniformUpdater final : public platform::IUnifromUpdater {
      private:
        types::OwnedRenderData resource;
        std::uint32_t buffer;
        std::uint32_t binding;

      public:
        OpenGLUniformUpdater(types::OwnedRenderData&& resource,
            std::uint32_t buffer,
            std::uint32_t binding) noexcept;
        void on_update(void) override;
        types::OwnedRenderData& get_resource(void) override;
    };
} // namespace enishi::renderer::opengl
