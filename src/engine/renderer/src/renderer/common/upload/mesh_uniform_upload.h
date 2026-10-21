#pragma once
#include <functional>
#include <platform/renderer/interface_renderer.h>
namespace enishi::renderer {
    using UniformWriter = std::function<platform::RenderResult<void>(platform::IResourceUpdater&)>;
    [[nodiscard]] platform::RenderResult<void> update_mesh_uniform(platform::IRenderer& renderer,
        types::RenderHandle mesh_handle,
        const std::string& name,
        const UniformWriter& write,
        const char* missing_uniform,
        const char* missing_updater,
        bool require_nonempty = true);
} // namespace enishi::renderer
