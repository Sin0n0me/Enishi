#include "animation_system.h"
#include "model_motion_loader.h"
#include <animation/animation_controller.h>

namespace enishi::core {
    foundation::Result<void, animation::AnimationError> AnimationSystem::play_vmd(
        types::HandleId entity, const std::filesystem::path& path, bool looping) {
        const auto model = this->registry->get<component::ModelComponent>(entity);
        if (model.is_none()) {
            return foundation::Error(
                animation::AnimationError::FailedConvert, "Model entity is missing");
        }
        auto loaded = load_model_motion(model.unwrap(), path);
        if (loaded.is_err()) {
            return loaded.propagation(animation::AnimationError::FailedConvert);
        }
        auto clip = std::make_shared<animation::AnimationClipData>(std::move(loaded).unwrap());
        clip->is_looping = looping;
        auto controller = std::make_shared<animation::AnimationController>();
        controller->add_clip(clip->name, clip);
        if (!controller->play(clip->name)) {
            return foundation::Error(
                animation::AnimationError::FailedConvert, "Cannot start model motion");
        }
        this->set_controller(entity, std::move(controller));
        return {};
    }
} // namespace enishi::core
