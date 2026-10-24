#include <component/animation_component.h>
#include <component/model_component.h>
#include <core/system/camera/camera_controller.h>
#include <core/system/window/window_animation_sync.h>
#include <cstdlib>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>
namespace {
    class SyncWindow final : public enishi::platform::IWindow {
      public:
        enishi::types::WindowPosition position{100, 200};
        bool fail_move = false;
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
    foundation::VoidResult<platform::WindowError> SyncWindow::init() noexcept {
        return {};
    }
    foundation::Option<platform::WindowHandle> SyncWindow::get_handle() const noexcept {
        return {};
    }
    foundation::Result<std::shared_ptr<platform::IOpenGLContext>, platform::RenderError>
    SyncWindow::create_opengl_context() {
        return foundation::Error(
            platform::RenderError::MakeError, "Window sync test has no OpenGL context");
    }
    foundation::Option<const platform::IInput*> SyncWindow::get_input() const noexcept {
        return {};
    }
    foundation::Option<types::WindowPosition> SyncWindow::get_position() const noexcept {
        return this->position;
    }
    foundation::VoidResult<platform::WindowError> SyncWindow::set_position(
        const types::WindowPosition& position) noexcept {
        if (this->fail_move) {
            return foundation::Error(
                platform::WindowError::FailedSetWinodwPosition, "Simulated failure");
        }
        this->position = position;
        return {};
    }
    foundation::Option<types::WindowSize> SyncWindow::get_size() const noexcept {
        constexpr std::uint32_t DIMENSION = 256;
        return types::WindowSize{DIMENSION, DIMENSION};
    }
    foundation::VoidResult<platform::WindowError> SyncWindow::set_size(
        const types::WindowSize&) noexcept {
        return {};
    }
    foundation::VoidResult<platform::WindowError> SyncWindow::set_title(
        const std::string&) noexcept {
        return {};
    }
    foundation::Option<std::string> SyncWindow::get_title() const noexcept {
        return {};
    }
    void SyncWindow::close() {
    }
    bool SyncWindow::should_close() const {
        return false;
    }
    void SyncWindow::poll_events() {
    }
    void SyncWindow::set_model_drag_handler(
        std::function<bool(types::ClientMousePosition)>, std::function<void()>) {
    }
    void SyncWindow::set_interactive_hit_test(std::function<bool(types::ClientMousePosition)>) {
    }
} // namespace

namespace {
    void require(bool condition, const char* message) {
        if (!condition) {
            std::cerr << message << '\n';
            std::exit(EXIT_FAILURE);
        }
    }
    void perspective_movement(core::WindowAnimationSync& sync,
        core::CameraController& camera,
        SyncWindow& window,
        component::AnimationComponent& pose,
        sub_system::WindowAnimationMode mode) {
        const auto projection = glm::perspectiveLH(glm::half_pi<float>(), 1.0f, 0.1f, 100.0f);
        require(camera.set_view_projection(glm::mat4(1), projection).is_ok(), "Perspective camera");
        const auto origin = window.position;
        pose.global[0][3] = glm::vec4(0, 0, 2, 1);
        const types::DeltaTime dt(1.0f / 60);
        sync.update(dt);
        pose.global[0][3] = glm::vec4(0.25f, 0.25f, 2, 1);
        sync.update(dt);
        // Perspective division at depth 2: 0.25/2*256/2 = 16 pixels.
        require(window.position.x == origin.x + 16 && window.position.y == origin.y - 16,
            "Perspective projection determines pixel displacement");
        const auto snapshot = sync.get_camera();
        const auto clip = snapshot.mvp * pose.global[0][3];
        const float expected =
            mode == sub_system::WindowAnimationMode::ReplaceModelMovement ? 0 : 0.125f;
        require(glm::length(glm::vec2(clip) / clip.w - glm::vec2(expected)) < 0.0001f,
            "Perspective display compensation uses homogeneous coordinates");
        const auto picked = glm::inverse(snapshot.mvp) * (clip / clip.w);
        require(glm::length(picked / picked.w - pose.global[0][3]) < 0.0001f,
            "Compensated camera maps picking coordinates back to the final pose");
    }

    void synchronization_mode(sub_system::WindowAnimationMode mode) {
        auto registry = std::make_shared<ecs::Registry>();
        auto window = std::make_shared<SyncWindow>();
        auto camera = std::make_shared<core::CameraController>();
        require(camera->set_view_projection(glm::mat4(1), glm::mat4(1)).is_ok(), "Identity camera");
        const auto entity = registry->create();
        component::ModelComponent model;
        model.bone_node.resize(1);
        component::AnimationComponent animation;
        animation.animation.resize(1);
        animation.global = {glm::mat4(1)};
        require(registry->insert(entity, std::move(model)).is_ok(), "Insert model");
        require(registry->insert(entity, std::move(animation)).is_ok(), "Insert animation");
        core::WindowAnimationSync sync(registry, window, camera);
        sub_system::IWindowAnimationSync& control = sync;
        require(control.follow(entity, 0, mode).is_ok(), "Select target through interface");
        const types::DeltaTime dt(1.0f / 60);
        sync.update(dt);
        auto& pose = registry->get<component::AnimationComponent>(entity).unwrap_mut();
        // 0.25 NDC * 256/2 = 32 pixels; positive model Y moves upward on screen.
        pose.global[0][3] = glm::vec4(0.25f, 0.25f, 0, 1);
        sync.update(dt);
        require(window->position.x == 132 && window->position.y == 168, "Follow final X/Y");
        const auto clip = sync.get_camera().mvp * pose.global[0][3];
        const float expected =
            mode == sub_system::WindowAnimationMode::ReplaceModelMovement ? 0 : 0.25f;
        require(glm::length(glm::vec2(clip) - glm::vec2(expected)) < 0.0001f,
            "Replace cancels screen movement; add preserves screen movement");
        require(pose.animation[0].position == glm::vec3(0), "Authored pose stays unchanged");
        sync.update(dt);
        require(window->position.x == 132 && window->position.y == 168, "No repeated displacement");
        // A failed move must not consume the sample or update display compensation.
        window->fail_move = true;
        pose.global[0][3].x += 0.25f;
        const auto prior_camera = sync.get_camera().mvp;
        sync.update(dt);
        require(window->position.x == 132 && sync.get_camera().mvp == prior_camera,
            "Failed move is atomic");
        window->fail_move = false;
        sync.update(dt);
        require(window->position.x == 164, "Failed displacement retried");
        // External/manual window movement must survive the next synchronized move.
        window->position.x += 10;
        pose.global[0][3].x += 0.25f;
        sync.update(dt);
        require(window->position.x == 206, "Manual window movement is preserved");
        pose.global[0][3].z += 1;
        sync.update(dt);
        require(window->position.x == 206, "Z alone never moves the window");
        const auto changed_view = glm::translate(glm::mat4(1), glm::vec3(0.5f, 0, 0));
        require(camera->set_view_projection(changed_view, glm::mat4(1)).is_ok(), "Camera change");
        sync.update(dt);
        require(window->position.x == 206, "Camera changes alone never move the window");
        auto& stored_model = registry->get<component::ModelComponent>(entity).unwrap_mut();
        stored_model.visible = false;
        pose.global[0][3].x += 2;
        sync.update(dt);
        stored_model.visible = true;
        sync.update(dt);
        require(window->position.x == 206, "Showing a model establishes a new baseline");
        require(control.follow(entity, 1, mode).is_err(), "Invalid bone rejected");
        control.stop();
        pose.global[0][3].x += 1;
        sync.update(dt);
        require(window->position.x == 206 && sync.get_camera().mvp == camera->get_camera().mvp,
            "Stopping removes compensation and disables following");
        require(control.follow(entity, 0, mode).is_ok(), "Restart");
        sync.update(dt);
        // Ten subpixel movements of 0.1 pixels accumulate to one pixel.
        for (int index = 0; index < 10; ++index) {
            pose.global[0][3].x += 0.1f / 128;
            sync.update(dt);
        }
        require(window->position.x == 207, "Fractional pixels accumulate");
        require(control.follow(entity, 0, mode).is_ok(), "Rebase for perspective");
        perspective_movement(sync, *camera, *window, pose, mode);
        const auto last_position = window->position;
        registry->destroy(entity);
        sync.update(dt);
        require(window->position.x == last_position.x && window->position.y == last_position.y,
            "Deleted targets do not move the window");
    }
} // namespace
void window_animation_sync_tests() {
    synchronization_mode(sub_system::WindowAnimationMode::ReplaceModelMovement);
    synchronization_mode(sub_system::WindowAnimationMode::AddWindowMovement);
}
