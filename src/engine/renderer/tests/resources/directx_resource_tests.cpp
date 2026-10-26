#include <renderer/directx/directx11/resource/native_gpu_resource.h>
#include <renderer/directx/directx11/resource/view/render_target_view.h>
#include <stdexcept>

using namespace enishi;
namespace {
    void require(bool value, const char* message) {
        if (!value) {
            throw std::runtime_error(message);
        }
    }
} // namespace

void backend_resource_tests() {
    renderer::directx::NativeGPUResource resources;
    auto* native_views = resources.get_native_view_accessor();
    auto* views = resources.get_view_accessor();
    const auto [native_handle, native_view] = native_views->make_native_render_target_view();
    auto wrapper = std::make_shared<renderer::directx::D3D11RenderTargetView>(
        types::RenderHandle{}, types::ImageViewDescription{});
    require(views->make_render_target_view(native_handle, std::move(wrapper)) == native_handle,
        "DirectX view registration must preserve the native handle");
    constexpr glm::vec4 COLOR{0.1f, 0.2f, 0.3f, 1.0f};
    views->get_render_target_view(native_handle).unwrap()->set_clear_color(COLOR);
    const auto& read = static_cast<const renderer::directx::NativeGPUResource&>(resources);
    require(read.get_view_accessor()
                    ->get_render_target_view(native_handle)
                    .unwrap()
                    ->get_clear_color() == COLOR,
        "DirectX view mutation must reach the registered wrapper");
    native_views->remove_native_view(native_handle);
    require(views->get_render_target_view(native_handle).is_none() &&
                views->get_render_target_views().empty(),
        "Native view removal must unregister the public interface");
    auto* states = resources.get_native_state_accessor();
    const auto [state, native_state] = states->make_native_sampler_state();
    require(read.get_state_accessor()->get_state_kind(state).unwrap() == types::StateKind::Sampler,
        "Native state allocation must register its kind");
    states->remove_native_state(state);
    require(read.get_state_accessor()->get_state_kind(state).is_none(),
        "Native state removal must unregister its kind");
    auto* buffers = resources.get_buffer_accessor();
    const auto [configurable, updater] = buffers->make_buffer();
    const auto [native_buffer, buffer] =
        resources.get_native_buffer_accessor()->make_native_buffer();
    resources.get_native_buffer_accessor()->remove_native_buffer(native_buffer);
    require(buffers->get_buffer(configurable).is_some(),
        "Native and configurable buffer handles must remain independent");
}
