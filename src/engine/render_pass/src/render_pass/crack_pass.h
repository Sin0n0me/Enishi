#pragma once
#include <cstdint>
#include <platform/asset/interface_shader_data_provider.h>
#include <platform/renderer/interface_render_command_encoder.h>
#include <platform/renderer/interface_renderer.h>
#include <platform/renderer/render_backend_description.h>
#include <platform/window/interface_window.h>
#include <render_pass/render_pass.h>
namespace enishi::render_pass {
    class CrackPass {
      public:
        struct Settings {
            float view_x;
            float view_y;
            float depth;
            std::uint32_t mask_size;
            float opacity;
        };

      private:
        std::shared_ptr<platform::IRenderer> renderer;
        std::shared_ptr<platform::IRenderCommandEncoder> encoder;
        RenderPass pass;
        types::RenderHandle mesh;
        Settings settings;

      public:
        explicit CrackPass(std::shared_ptr<platform::IRenderer> renderer,
            std::shared_ptr<platform::IRenderCommandEncoder> encoder,
            Settings settings);
        platform::RenderResult<void> initialize(const platform::IWindow& window,
            const platform::IShaderDataProvider& shaders,
            const platform::RenderBackendDescription& backend,
            std::shared_ptr<types::TextureData> mask);
        platform::RenderResult<void> draw(float opacity);

      private:
        platform::RenderResult<void> update_uniform(float opacity);
        void submit();
    };
} // namespace enishi::render_pass
