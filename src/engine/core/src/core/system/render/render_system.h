#pragma once
#include "../interface_system.h"
#include <ecs/registory.h>
#include <engine_types/renderer/render_graph.h>
#include <foundation/str/str.h>
#include <unordered_map>

namespace enishi::core {
    class RenderSystem : public ISystem {
      private:
        std::shared_ptr<ecs::Registory> registory;
        types::RenderGraph render_graph;
        std::unordered_map<foundation::UTF8, std::uint64_t> name_to_index;

        explicit RenderSystem(void) = delete;

      public:
        explicit RenderSystem(std::shared_ptr<ecs::Registory> registory);

        void update(const types::DeltaTime& delta_time) override;

        const types::RenderGraph& get_render_graph(void) const;

        void add_render_pass(foundation::UTF8&& pass_name, types::RenderPass&& render_pass);

        types::RenderPass& get_render_pass(const foundation::UTF8& pass_name);

      private:
        void add_command();

        void update_command();
    };
} // namespace enishi::core
