#include <cstdlib>
#include <iostream>
#include <renderer/common/resource_accessor.h>
#include <renderer/common/vertex_buffer_updater.h>
#include <stdexcept>

using namespace enishi;
void backend_resource_tests();

namespace {
    void require(bool value, const char* message) {
        if (!value) {
            throw std::runtime_error(message);
        }
    }

    void buffer_tests() {
        renderer::ResourceAccessor resources;
        platform::IRenderResourceAccessor& access = resources;
        auto* buffers = access.get_resource_accessor()->get_buffer_accessor();
        const auto& read = static_cast<const renderer::ResourceAccessor&>(resources);
        bool uploaded = false;
        constexpr std::size_t BYTE_COUNT = 1;
        constexpr std::byte VALUE{0x42};
        const auto updater = std::make_shared<renderer::VertexBufferUpdater>(
            types::OwnedRenderData(std::vector<std::byte>(BYTE_COUNT, VALUE), BYTE_COUNT),
            [&](const types::RenderData& data) {
                require(data.bytes.front() == VALUE, "Retrieved updater lost its data");
                uploaded = true;
            });
        const auto [handle, stored] = buffers->make_buffer();
        buffers->add_interface(handle, updater);
        require(stored == updater, "Registration must preserve the returned slot");
        require(read.get_buffer_accessor()->get_bufer(handle).unwrap() == updater,
            "Const accessor must resolve the same updater");
        buffers->get_buffer(handle).unwrap()->on_update();
        require(uploaded, "Retrieved updater must perform its upload");
        buffers->remove_buffer(handle);
        require(buffers->get_buffer(handle).is_none(), "Removed updater must not be returned");
        require(read.get_buffer_accessor()->get_bufer(handle).is_none(),
            "Const accessor must not return removed updater");
    }

    void view_and_metadata_tests() {
        renderer::ResourceAccessor resources;
        constexpr types::HandleId VIEW_HANDLE{42};
        auto* views = resources.get_resource_accessor()->get_view_accessor();
        // Null wrappers suffice here: registration must distinguish the view kinds by handle.
        require(views->make_render_target_view(VIEW_HANDLE, {}) == VIEW_HANDLE,
            "View registration must preserve the native handle");
        require(views->get_view_type(VIEW_HANDLE).unwrap() == types::ImageViewType::RenderTarget,
            "View type must survive registration");
        require(views->get_depth_stencil_view(VIEW_HANDLE).is_none(),
            "Wrong view kind must not resolve");
        resources.remove_view(VIEW_HANDLE);
        require(views->get_render_target_view(VIEW_HANDLE).is_none() &&
                    views->get_render_target_views().empty(),
            "View removal must update both lookup and enumeration");
        require(views->make_depth_stencil_view(VIEW_HANDLE, {}) == VIEW_HANDLE,
            "Reused view registration must preserve the native handle");
        require(views->get_view_type(VIEW_HANDLE).unwrap() == types::ImageViewType::DepthStencil,
            "Reused native handle must resolve the new kind");
        resources.add_state(VIEW_HANDLE, types::StateKind::Sampler);
        require(resources.get_state_accessor()->get_state_kind(VIEW_HANDLE).unwrap() ==
                    types::StateKind::Sampler,
            "State kind must be available through common accessor");
        resources.remove_state(VIEW_HANDLE);
        require(resources.get_state_kind(VIEW_HANDLE).is_none(), "Removed state must be absent");
        const auto [mesh, mesh_data] = resources.make_mesh_handles();
        require(&resources.get_mesh_accessor()->get_mesh_handle(mesh).unwrap() == &mesh_data,
            "Mesh accessor must resolve registered mesh data");
        resources.remove_mesh_handles(mesh);
        require(resources.get_mesh_handle(mesh).is_none(), "Removed mesh must be absent");
    }
} // namespace

int main() {
    try {
        buffer_tests();
        view_and_metadata_tests();
        backend_resource_tests();
        return EXIT_SUCCESS;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return EXIT_FAILURE;
    }
}
