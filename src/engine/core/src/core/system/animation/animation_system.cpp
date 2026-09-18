#include "animation_system.h"
#include <animation/keyframe_interpolator.h>

namespace enishi::core {
    AnimationSystem::AnimationSystem(const std::shared_ptr<ecs::Registry> registry)
        : registry(registry) {
    }

    void AnimationSystem::set_controller(
        const types::HandleId entity, std::shared_ptr<animation::IAnimationController> controller) {
        if (controller) {
            this->controllers.insert_or_assign(entity, std::move(controller));
            return;
        }
        this->controllers.erase(entity);
    }

    void AnimationSystem::remove_controller(const types::HandleId& entity) {
        this->controllers.erase(entity);
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
            if (!controller) {
                continue;
            }

            controller->update(delta_time.to_float_second());
            this->apply_clip(animation, *controller);
        }
    }

    void AnimationSystem::render(void) const {
    }

    void AnimationSystem::apply_clip(component::AnimationComponent& animation,
        const animation::IAnimationController& controller) const {
        const auto* const clip = controller.get_active_clip();
        if (!clip) {
            return;
        }

        for (auto& bone : animation.animation) {
            bone.position = glm::vec3(0.0f);
            bone.rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
            bone.scale = glm::vec3(1.0f);
        }

        const float time = controller.get_time();
        for (const auto& track : clip->bone_tracks) {
            if (track.bone_index >= animation.animation.size()) {
                continue;
            }

            auto& bone = animation.animation[track.bone_index];
            if (!track.positions.times.empty()
                && track.positions.times.size() == track.positions.values.size()) {
                bone.position = animation::KeyframeInterpolator::sample(track.positions, time);
            }
            if (!track.rotations.times.empty()
                && track.rotations.times.size() == track.rotations.values.size()) {
                bone.rotation = animation::KeyframeInterpolator::sample(track.rotations, time);
            }
            if (!track.scales.times.empty() && track.scales.times.size() == track.scales.values.size()) {
                bone.scale = animation::KeyframeInterpolator::sample(track.scales, time);
            }
        }
    }
} // namespace enishi::core
