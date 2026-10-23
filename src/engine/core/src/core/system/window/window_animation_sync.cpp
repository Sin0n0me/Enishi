#include "window_animation_sync.h"
#include <cmath>
#include <component/animation_component.h>
#include <component/model_component.h>
#include <component/physics_bodies_component.h>
#include <foundation/log/logger.h>
#include <limits>
#include <utility>

namespace enishi::core {
    WindowAnimationSync::WindowAnimationSync(std::shared_ptr<ecs::Registry> registry,
        std::shared_ptr<platform::IWindow> window,
        std::shared_ptr<sub_system::ICameraController> camera)
        : registry(std::move(registry))
        , window(std::move(window))
        , camera(std::move(camera)) {
    }

    foundation::VoidResult<sub_system::WindowAnimationError> WindowAnimationSync::follow(
        types::HandleId model, types::BoneIndex bone, sub_system::WindowAnimationMode mode) {
        const auto component = this->registry->get<component::ModelComponent>(model);
        if (!this->registry->is_alive(model) || component.is_none() ||
            !(bone < component.unwrap().bone_node.size()) ||
            !this->registry->has<component::AnimationComponent>(model) ||
            (mode != sub_system::WindowAnimationMode::ReplaceModelMovement &&
                mode != sub_system::WindowAnimationMode::AddWindowMovement)) {
            return foundation::Error(sub_system::WindowAnimationError::InvalidTarget,
                "Window animation synchronization requires a valid model, bone and mode");
        }
        this->stop();
        this->target = Target{model, bone, mode};
        return {};
    }

    void WindowAnimationSync::stop() {
        this->target.reset();
        this->previous.reset();
        this->remainder = glm::dvec2(0);
        this->compensation = glm::dvec2(0);
    }

    std::optional<glm::vec3> WindowAnimationSync::final_position() const {
        if (!this->target.has_value() || !this->registry->is_alive(this->target->model)) {
            return std::nullopt;
        }
        const auto model = this->registry->get<component::ModelComponent>(this->target->model);
        const auto animation =
            this->registry->get<component::AnimationComponent>(this->target->model);
        const auto bodies =
            this->registry->get<component::PhysicsBodiesComponent>(this->target->model);
        if (model.is_none() || !model.unwrap().visible || animation.is_none() ||
            !(this->target->bone < animation.unwrap().global.size()) ||
            (bodies.is_some() && !bodies.unwrap().enabled)) {
            return std::nullopt;
        }
        const auto position = glm::vec3(animation.unwrap().global[this->target->bone][3]);
        if (!std::isfinite(position.x) || !std::isfinite(position.y) ||
            !std::isfinite(position.z)) {
            return std::nullopt;
        }
        return position;
    }

    glm::mat4 WindowAnimationSync::screen_shift() const {
        glm::mat4 shift(1);
        const auto size = this->window->get_size();
        if (size.is_some() && size.unwrap().width > 0 && size.unwrap().height > 0) {
            // Pixel offset maps to clip offset times w: dx_ndc=-2*dx/W, dy_ndc=2*dy/H.
            shift[3][0] = static_cast<float>(-2 * this->compensation.x / size.unwrap().width);
            shift[3][1] = static_cast<float>(2 * this->compensation.y / size.unwrap().height);
        }
        return shift;
    }

    types::UniformCamera WindowAnimationSync::get_camera() const {
        auto snapshot = this->camera->get_camera();
        snapshot.projection = this->screen_shift() * snapshot.projection;
        snapshot.mvp = snapshot.projection * snapshot.view * snapshot.world;
        return snapshot;
    }

    foundation::VoidResult<sub_system::CameraError> WindowAnimationSync::set_view_projection(
        const glm::mat4& view, const glm::mat4& projection) {
        return this->camera->set_view_projection(
            view, glm::inverse(this->screen_shift()) * projection);
    }

    void WindowAnimationSync::reset() {
        this->stop();
        this->camera->reset();
    }

    void WindowAnimationSync::synchronize(const glm::vec3& position) {
        const auto size = this->window->get_size();
        const auto origin = this->window->get_position();
        if (size.is_none() || origin.is_none() || !(size.unwrap().width > 0) ||
            !(size.unwrap().height > 0)) {
            this->previous.reset();
            return;
        }
        const auto snapshot = this->camera->get_camera();
        // Project both endpoints with the same current camera/depth: zoom and Z motion alone
        // must not move the window. screen=(ndc.x*W/2, -ndc.y*H/2).
        const auto before =
            snapshot.mvp * glm::vec4(this->previous->x, this->previous->y, position.z, 1);
        const auto after = snapshot.mvp * glm::vec4(position, 1);
        if (!(before.w > 0) || !(after.w > 0)) {
            this->previous.reset();
            return;
        }
        const auto delta = glm::dvec2(after / after.w - before / before.w) *
                           glm::dvec2(size.unwrap().width * 0.5, size.unwrap().height * -0.5);
        const auto pending = delta + this->remainder;
        const auto step = glm::round(pending);
        const auto destination = glm::dvec2(origin.unwrap().x, origin.unwrap().y) + step;
        constexpr double MIN_POSITION = (std::numeric_limits<std::int32_t>::min)();
        constexpr double MAX_POSITION = (std::numeric_limits<std::int32_t>::max)();
        if (!std::isfinite(destination.x) || !std::isfinite(destination.y) ||
            destination.x < MIN_POSITION || destination.x > MAX_POSITION ||
            destination.y < MIN_POSITION || destination.y > MAX_POSITION) {
            foundation::Logger::warning("Window animation position is outside the supported range");
            this->previous.reset();
            this->remainder = glm::dvec2(0);
            return;
        }
        if (step != glm::dvec2(0)) {
            const auto result =
                this->window->set_position({static_cast<std::int32_t>(destination.x),
                    static_cast<std::int32_t>(destination.y)});
            if (result.is_err()) {
                foundation::Logger::warning(result.unwrap_err().get_message());
                return;
            }
        }
        // Retain fractional pixels, but compensate only the displacement actually submitted.
        this->remainder = pending - step;
        if (this->target->mode == sub_system::WindowAnimationMode::ReplaceModelMovement) {
            this->compensation += step;
        }
        this->previous = position;
    }

    void WindowAnimationSync::update(const types::DeltaTime&) {
        const auto position = this->final_position();
        if (!position.has_value()) {
            this->previous.reset();
            this->remainder = glm::dvec2(0);
            this->compensation = glm::dvec2(0);
            if (this->target.has_value() && !this->registry->is_alive(this->target->model)) {
                this->target.reset();
            }
            return;
        }
        if (this->previous.has_value()) {
            this->synchronize(*position);
        } else {
            this->previous = position;
        }
    }
    void WindowAnimationSync::pre_update() {
    }
    void WindowAnimationSync::post_update() {
    }
    void WindowAnimationSync::render() const {
    }
} // namespace enishi::core
