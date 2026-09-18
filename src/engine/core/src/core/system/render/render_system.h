#pragma once
#include "../interface_system.h"
#include <core/errors/errors.h>
#include <ecs/registry.h>
#include <engine_types/renderer/render_graph.h>
#include <foundation/str/str.h>
#include <platform/renderer/interface_render_command_encoder.h>
#include <platform/renderer/interface_render_pass.h>
#include <platform/renderer/interface_renderer.h>
#include <unordered_map>

namespace enishi::core {
    class RenderSystem : public ISystem {
      private:
        std::shared_ptr<ecs::Registry> registry;
        std::shared_ptr<platform::IRenderer> renderer;
        std::shared_ptr<platform::IRenderCommandEncoder> encoder;
        std::vector<std::shared_ptr<platform::IRenderPass>> render_passes;

        explicit RenderSystem(void) = delete;

      public:
        explicit RenderSystem(std::shared_ptr<ecs::Registry> registry,
            std::shared_ptr<platform::IRenderer> renderer,
            std::shared_ptr<platform::IRenderCommandEncoder> encoder);

      public:
        void set_render_passes(std::vector<std::shared_ptr<platform::IRenderPass>>&& render_passes);
        [[nodiscard]] types::RenderPass& get_render_pass(const foundation::UTF8& pass_name);

        [[nodiscard]] std::shared_ptr<platform::IRenderer> get_renderer(void) const;
        [[nodiscard]] std::shared_ptr<platform::IRenderCommandEncoder> get_render_command_encoder(
            void) const;

      public:
        bool should_close(void) override;
        void pre_update(void) override;
        void post_update(void) override;
        void update(const types::DeltaTime& delta_time) override;
        void render(void) const override;

      private:
        void submit_render_graph(void) const;
        void present(void) const;
        void execute(const types::DrawCommand& command) const;
        void bind(const types::RenderHandle& render_handle) const;
    };
} // namespace enishi::core
