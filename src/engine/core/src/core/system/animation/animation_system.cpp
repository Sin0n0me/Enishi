#include "animation_system.h"
#include "clip_sampler.h"

namespace enishi::core {
    AnimationSystem::AnimationSystem(const std::shared_ptr<ecs::Registry> registry)
        : registry(registry) {
    }

    void AnimationSystem::set_controller(
        const types::HandleId entity, std::shared_ptr<animation::IAnimationController> controller) {
        if (controller != nullptr) {
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
            if (controller == nullptr) {
                continue;
            }

            controller->update(delta_time.to_float_second());
            const auto* clip = controller->get_active_clip();
            if (clip == nullptr) {
                continue;
            }
            auto morph = this->registry->get<component::MorphComponent>(entity);
            auto ik = this->registry->get<component::IKComponent>(entity);
            sample_model_clip(*clip,
                controller->get_time(),
                animation,
                morph.is_some() ? &morph.unwrap_mut() : nullptr,
                ik.is_some() ? &ik.unwrap_mut() : nullptr);
        }
    }

    void AnimationSystem::render(void) const {
    }

} // namespace enishi::core
