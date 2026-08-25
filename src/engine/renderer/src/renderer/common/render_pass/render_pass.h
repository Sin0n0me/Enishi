#pragma once
#include <foundation/option/option.h>
#include <foundation/str/str.h>
#include <memory>
#include <platform/renderer/interface_render_pass.h>
#include <platform/renderer/updater/interface_resource_updater.h>
#include <platform/renderer/updater/interface_uniform_updater.h>
#include <unordered_map>

namespace enishi::renderer {
    class RenderPass : public platform::IRenderPass {
      private:
        types::RenderHandle render_target;
        std::vector<types::DrawCommand> commands;
        std::unordered_map<foundation::UTF8, std::size_t> mesh_name_to_index;
        std::vector<std::shared_ptr<platform::IResourceUpdater>> resource_updater;

      public:
        foundation::VoidResult<platform::RenderError> make_render_pass(
            const types::PipelineDescription& description) noexcept;

        void add_mesh(const foundation::UTF8& mesh_name, const types::RenderHandle handle);

        // 舞フレーム更新する場合
        void add_updater(std::shared_ptr<platform::IResourceUpdater> updater);

      public:
        std::span<const types::DrawCommand> get_commands(void) const noexcept override;
        types::RenderHandle get_render_target(void) const noexcept override;
        void update(void) override;

      private:
        void add_command(const types::RenderHandle handle);
    };
} // namespace enishi::renderer