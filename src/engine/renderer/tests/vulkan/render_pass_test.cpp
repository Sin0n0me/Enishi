#include <array>
#include <component/model_component.h>
#include <core/system/asset/asset_system.h>
#include <core/system/asset/shader/shader_data_provider.h>
#include <core/system/render/shadow_upload.h>
#include <cstring>
#include <iostream>
#include <platform/window/interface_window.h>
#include <render_pass/constructor/background/background_render_pass_constructor.h>
#include <render_pass/constructor/model/model_render_pass_constructor.h>
#include <render_pass/constructor/shadow/shadow_map_render_pass_constructor.h>
#include <render_pass/render_pass_orchestra.h>

namespace {
    class SizeWindow final : public enishi::platform::IWindow {
      public:
        enishi::foundation::VoidResult<enishi::platform::WindowError> init() noexcept override;
        enishi::foundation::Option<enishi::platform::WindowHandle>
        get_handle() const noexcept override;
        enishi::foundation::Result<std::shared_ptr<enishi::platform::IOpenGLContext>,
            enishi::platform::RenderError>
        create_opengl_context() override;
        enishi::foundation::Option<const enishi::platform::IInput*>
        get_input() const noexcept override;
        enishi::foundation::Option<enishi::types::WindowPosition>
        get_position() const noexcept override;
        enishi::foundation::VoidResult<enishi::platform::WindowError> set_position(
            const enishi::types::WindowPosition&) noexcept override;
        enishi::foundation::Option<enishi::types::WindowSize> get_size() const noexcept override;
        enishi::foundation::VoidResult<enishi::platform::WindowError> set_size(
            const enishi::types::WindowSize&) noexcept override;
        enishi::foundation::VoidResult<enishi::platform::WindowError> set_title(
            const std::string&) noexcept override;
        enishi::foundation::Option<std::string> get_title() const noexcept override;
        void close() override;
        bool should_close() const override;
        void poll_events() override;
        void set_model_drag_handler(std::function<bool(enishi::types::ClientMousePosition)>,
            std::function<void()>) override;
        void set_interactive_hit_test(
            std::function<bool(enishi::types::ClientMousePosition)>) override;
    };
    using namespace enishi;
    foundation::VoidResult<platform::WindowError> SizeWindow::init() noexcept {
        return {};
    }
    foundation::Option<platform::WindowHandle> SizeWindow::get_handle() const noexcept {
        return {};
    }
    foundation::Result<std::shared_ptr<platform::IOpenGLContext>, platform::RenderError>
    SizeWindow::create_opengl_context() {
        return foundation::Error(
            platform::RenderError::MakeError, "Vulkan test has no OpenGL context");
    }
    foundation::Option<const platform::IInput*> SizeWindow::get_input() const noexcept {
        return {};
    }
    foundation::Option<types::WindowPosition> SizeWindow::get_position() const noexcept {
        return {};
    }
    foundation::VoidResult<platform::WindowError> SizeWindow::set_position(
        const types::WindowPosition&) noexcept {
        return {};
    }
    foundation::Option<types::WindowSize> SizeWindow::get_size() const noexcept {
        constexpr std::uint32_t DIMENSION = 256;
        return types::WindowSize{DIMENSION, DIMENSION};
    }
    foundation::VoidResult<platform::WindowError> SizeWindow::set_size(
        const types::WindowSize&) noexcept {
        return {};
    }
    foundation::VoidResult<platform::WindowError> SizeWindow::set_title(
        const std::string&) noexcept {
        return {};
    }
    foundation::Option<std::string> SizeWindow::get_title() const noexcept {
        return {};
    }
    void SizeWindow::close() {
    }
    bool SizeWindow::should_close() const {
        return false;
    }
    void SizeWindow::poll_events() {
    }
    void SizeWindow::set_model_drag_handler(
        std::function<bool(types::ClientMousePosition)>, std::function<void()>) {
    }
    void SizeWindow::set_interactive_hit_test(std::function<bool(types::ClientMousePosition)>) {
    }
} // namespace

bool render_pass_test(const std::shared_ptr<enishi::platform::IRenderer>& renderer) {
    enishi::core::AssetSystem assets;
    auto provider = std::make_shared<enishi::core::ShaderDataProvider>(assets.get_asset_system());
    enishi::render_pass::RenderPassOrchestra orchestra(renderer, provider);
    orchestra.add_constructor(
        std::make_shared<enishi::render_pass::BackgroundRenderPassConstructor>());
    orchestra.add_constructor(std::make_shared<enishi::render_pass::ModelRenderPassConstructor>());
    orchestra.add_constructor(
        std::make_shared<enishi::render_pass::ShadowMapRenderPassConstructor>());
    SizeWindow window;
    auto result = orchestra.make_render_passes(&window);
    if (result.is_err()) {
        std::cerr << result.unwrap_err().get_message() << '\n';
        return false;
    }
    orchestra.set_render_passes({enishi::render_pass::ModelRenderPassConstructor::RENDER_PASS_NAME,
        enishi::render_pass::BackgroundRenderPassConstructor::RENDER_PASS_NAME,
        enishi::render_pass::ShadowMapRenderPassConstructor::RENDER_PASS_NAME});
    const auto shadow = orchestra.get_render_pass(
        enishi::render_pass::ShadowMapRenderPassConstructor::RENDER_PASS_NAME);
    if (shadow.is_none() || !shadow.unwrap()->get_sampled_depth().is_valid()) {
        return false;
    }
    const auto background = orchestra.get_render_pass(
        enishi::render_pass::BackgroundRenderPassConstructor::RENDER_PASS_NAME);
    if (background.is_none() || !background.unwrap()->get_shadow_receiver().is_valid()) {
        return false;
    }
    enishi::ecs::Registry registry;
    const auto entity = registry.create();
    const auto wall = background.unwrap()->get_shadow_receiver();
    auto model = registry.emplace<enishi::component::ModelComponent>(entity);
    if (model.is_err()) {
        return false;
    }
    model.unwrap_mut().render_handle = wall;
    model.unwrap_mut().shadow_model_bounds.include({-1, -1, 0});
    model.unwrap_mut().shadow_model_bounds.include({1, 1, 1});
    const enishi::types::UniformCamera camera{
        glm::mat4(1), glm::mat4(1), glm::mat4(1), glm::mat4(1)};
    const std::array receivers{wall};
    auto uploaded = enishi::core::upload_shadow(*renderer, registry, camera, receivers);
    if (uploaded.is_err()) {
        std::cerr << uploaded.unwrap_err().get_message() << '\n';
        return false;
    }
    const auto mapped = renderer->get_handle_mapper()->get(wall).unwrap().resource;
    auto* resources = renderer->get_resource_accessor()->get_resource_accessor();
    const auto mesh = resources->get_mesh_accessor()->get_mesh_handle(mapped).unwrap();
    const auto buffer = resources->get_buffer_accessor()
                            ->get_buffer(mesh.uniform_buffers.at("Camera").front())
                            .unwrap();
    enishi::types::UniformCamera uploaded_camera{};
    std::memcpy(&uploaded_camera,
        buffer->get_resource().get_render_data().raw_data(),
        sizeof(uploaded_camera));
    if (!(uploaded_camera.world[3].z > 1)) {
        return false;
    }
    model.unwrap_mut().visible = false;
    uploaded = enishi::core::upload_shadow(*renderer, registry, camera, receivers);
    if (uploaded.is_err()) {
        return false;
    }
    std::memcpy(&uploaded_camera,
        buffer->get_resource().get_render_data().raw_data(),
        sizeof(uploaded_camera));
    if (glm::length(glm::vec3(uploaded_camera.world[0])) != 0) {
        return false;
    }
    std::cout << "Generated shader loading and application render-pass construction passed\n";
    return true;
}
