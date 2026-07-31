#include "render_system.h"
#include <component/animation_component.h>
#include <component/model_component.h>

namespace enishi::core {
    RenderSystem::RenderSystem(std::shared_ptr<ecs::Registory> registory)
        : registory(registory) {
    }

    void RenderSystem::update(const types::DeltaTime& delta_time) {
        auto view =
            this->registory->view<component::AnimationComponent, component::ModelComponent>();

        for (auto [entity, animation, model] : view) {
        }
    }

    const types::RenderGraph& core::RenderSystem::get_render_graph(void) const {
        return this->render_graph;
    }

    void core::RenderSystem::add_render_pass(
        foundation::UTF8&& pass_name, types::RenderPass&& render_pass) {
        auto& passes = this->render_graph.passes;
        const auto iter = this->name_to_index.find(pass_name);
        if (iter == this->name_to_index.end()) {
            const auto index = passes.size();
            passes.emplace_back(std::move(render_pass));
            this->name_to_index[pass_name] = index;
            return;
        }

        const auto index = iter->second;
        passes[index] = std::move(render_pass);
    }

    void core::RenderSystem::add_command() {
    }
} // namespace enishi::core