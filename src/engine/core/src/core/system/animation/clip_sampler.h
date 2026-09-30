#pragma once
#include <animation/clip_data/animation.h>
#include <component/animation_component.h>
#include <component/ik_component.h>
#include <component/morph_component.h>

namespace enishi::core {
    void sample_model_clip(const animation::AnimationClipData& clip,
        float time,
        component::AnimationComponent& pose,
        component::MorphComponent* morph,
        component::IKComponent* ik);
} // namespace enishi::core
