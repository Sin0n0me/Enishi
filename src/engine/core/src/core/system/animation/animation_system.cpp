#include "animation_system.h"
#include <animation/clip_sampler.h>
#include <component/physics_bodies_component.h>

namespace enishi::core {
    AnimationSystem::AnimationSystem(const std::shared_ptr<ecs::Registry> registry)
        : registry(registry) {
    }

    void AnimationSystem::set_controller(
        const types::HandleId entity, std::shared_ptr<animation::IAnimationController> controller) {
        this->playback_states.erase(entity);
        if (controller != nullptr) {
            auto bodies = this->registry->get<component::PhysicsBodiesComponent>(entity);
            if (bodies.is_some()) {
                bodies.unwrap_mut().pose_reset_requested = true;
            }
            this->controllers.insert_or_assign(entity, std::move(controller));
            return;
        }
        this->controllers.erase(entity);
    }

    void AnimationSystem::remove_controller(const types::HandleId& entity) {
        this->controllers.erase(entity);
        this->playback_states.erase(entity);
    }

    std::shared_ptr<animation::IAnimationController> AnimationSystem::get_controller(
        const types::HandleId& entity) const {
        const auto iter = this->controllers.find(entity);
        if (iter == this->controllers.end()) {
            return {};
        }
        return iter->second;
    }

    bool AnimationSystem::should_close(void) {
        return false;
    }

    void AnimationSystem::pre_update(void) {
    }

    void AnimationSystem::post_update(void) {
    }

    void AnimationSystem::update(const types::DeltaTime& delta_time) {
        for (auto [entity, animation, model] :
            this->registry->view<component::AnimationComponent, component::ModelComponent>()) {
            const auto controller = this->get_controller(entity);
            animation.motion_playing = false;
            if (controller == nullptr) {
                continue;
            }

            auto& playback = this->playback_states[entity];
            const bool changed = !playback.initialized ||
                                 playback.clip != controller->get_active_clip() ||
                                 controller->get_time() < playback.time ||
                                 (controller->is_playing() && !playback.playing);
            if (changed) {
                ++animation.motion_revision;
            }
            // Sample the new clip's initial pose before advancing it. Skinning holds the
            // controller clock until its pose transition has reached that sample.
            if (!changed && !animation.pose_transition_active) {
                controller->update(delta_time.to_float_second());
            }
            const auto* clip = controller->get_active_clip();
            animation.motion_playing = clip != nullptr && controller->is_playing();
            playback = {clip, controller->get_time(), controller->is_playing(), true};
            if (clip == nullptr) {
                continue;
            }
            if (controller->get_time() < animation.elapsed_time) {
                auto bodies = this->registry->get<component::PhysicsBodiesComponent>(entity);
                if (bodies.is_some()) {
                    bodies.unwrap_mut().pose_reset_requested = true;
                }
            }
            auto morph = this->registry->get<component::MorphComponent>(entity);
            auto ik = this->registry->get<component::IKComponent>(entity);
            animation::sample_model_clip(*clip,
                controller->get_time(),
                animation,
                morph.is_some() ? &morph.unwrap_mut() : nullptr,
                ik.is_some() ? &ik.unwrap_mut() : nullptr);
        }
    }

    void AnimationSystem::render(void) const {
    }

} // namespace enishi::core
