#pragma once
#include "../interface_system.h"
#include "interface_render_pass_construstor.h"
#include <ecs/registory.h>
#include <engine_types/renderer/render_graph.h>
#include <foundation/str/str.h>
#include <platform/renderer/interface_render_command_encoder.h>
#include <platform/renderer/interface_renderer.h>
#include <unordered_map>

namespace enishi::core {
    class RenderSystem : public ISystem {
      private:
        template <typename T> using NameMap = std::unordered_map<foundation::UTF8, T>;

        std::shared_ptr<ecs::Registory> registory;
        std::shared_ptr<platform::IRenderer> renderer;
        std::shared_ptr<platform::IRenderCommandEncoder> encoder;
        std::vector<std::shared_ptr<platform::IRenderPass>> render_passes;
        NameMap<std::uint64_t> name_to_index;
        NameMap<std::shared_ptr<IRenderPassConstructor>> name_to_constructor;

        explicit RenderSystem(void) = delete;

      public:
        explicit RenderSystem(std::shared_ptr<ecs::Registory> registory,
            std::shared_ptr<platform::IRenderer> renderer,
            std::shared_ptr<platform::IRenderCommandEncoder> encoder);

      public:
        foundation::VoidResult<SystemError> add_render_pass_constructor(
            foundation::UTF8&& pass_name,
            std::shared_ptr<IRenderPassConstructor> render_pass_constructor);

        foundation::VoidResult<SystemError> create_render_passes(
            assets_system::IAssetSystem* const asset_system);

        types::RenderPass& get_render_pass(const foundation::UTF8& pass_name);

        std::shared_ptr<platform::IRenderer> get_renderer(void) const;
        std::shared_ptr<platform::IRenderCommandEncoder> get_render_command_encoder(void) const;

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
