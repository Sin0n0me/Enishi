#pragma once
#include <effect/crack/crack_settings.h>
#include <platform/asset/interface_shader_data_provider.h>
#include <platform_impl/renderer/renderer_factory.h>
#include <render_pass/render_pass.h>
namespace enishi::platform_impl {
    class CrackPass {
      private:
        RendererInstance graphics;
        render_pass::RenderPass pass;
        types::RenderHandle mesh;
        effect::CrackSettings settings;

      public:
        explicit CrackPass(RendererInstance graphics, effect::CrackSettings settings);
        platform::RenderResult<void> initialize(const platform::IWindow& window,
            const platform::IShaderDataProvider& shaders,
            std::shared_ptr<types::TextureData> mask);
        platform::RenderResult<void> draw(float opacity);

      private:
        platform::RenderResult<void> update_uniform(float opacity);
        void submit();
    };
} // namespace enishi::platform_impl
