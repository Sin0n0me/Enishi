#pragma once
#include "../interface_system.h"
#include <ecs/registory.h>
#include <engine_types/renderer/render_graph.h>
#include <foundation/str/str.h>
#include <platform/renderer/interface_render_command_encoder.h>
#include <platform/renderer/interface_renderer.h>
#include <unordered_map>

namespace enishi::core {
    class RenderSystem : public ISystem {
      private:
        std::shared_ptr<ecs::Registory> registory;
        std::unique_ptr<platform::IRenderer> renderer;
        std::unique_ptr<platform::IRenderCommandEncoder> encoder;
        types::RenderGraph render_graph;
        std::unordered_map<foundation::UTF8, std::uint64_t> name_to_index;

        explicit RenderSystem(void) = delete;

      public:
        explicit RenderSystem(std::shared_ptr<ecs::Registory> registory);

        void update(const types::DeltaTime& delta_time) override;

        void add_render_pass(foundation::UTF8&& pass_name, types::RenderPass&& render_pass);

        types::RenderPass& get_render_pass(const foundation::UTF8& pass_name);

        platform::IRenderer* get_renderer(void);

      private:
        void draw(void) const;
        void submit_render_graph(const types::RenderGraph& graph) const;
        void present(void) const;
        void execute(const types::DrawCommand& command) const;
        void bind(const types::RenderHandle& render_handle) const;

        void add_command();

        void update_command();
    };
} // namespace enishi::core
