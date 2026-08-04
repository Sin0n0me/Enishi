#include "resource_editor.h"

namespace enishi::renderer::directx {
    void ResourceEditor::add_render_target(std::shared_ptr<platform::IRenderTargetView> rtv) {
        const auto handle = rtv->get_handle();

        this->handle_to_index[handle.id] = Resource{
            .type = ResourceType::RenderTarget,
            .index = this->render_targets.size(),
        };

        this->render_targets.emplace_back(rtv);
    }

    foundation::Option<std::shared_ptr<platform::IRenderTargetView>>
    ResourceEditor::get_render_target(const types::HandleId handle) const {
        const auto& iter = this->handle_to_index.find(handle);
        if (iter == this->handle_to_index.end()) {
            return {};
        }
        const auto& resource = iter->second;
        if (resource.type != ResourceType::RenderTarget) {
            return {};
        }
        if (this->render_targets.size() < resource.index + 1) {
            return {};
        }

        return this->render_targets[resource.index];
    }

    const std::vector<std::shared_ptr<platform::IRenderTargetView>>&
    ResourceEditor::get_render_targets(void) const {
        return this->render_targets;
    }
} // namespace enishi::renderer::directx