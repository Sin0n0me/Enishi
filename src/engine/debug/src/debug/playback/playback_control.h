#pragma once
#include <engine_types/handle/handle_type.h>
#include <unordered_set>

namespace enishi::debug {
    struct PlaybackControl {
        static constexpr float FRAMES_PER_SECOND = 60.0f;
        // One debug frame advances the motion clock by 1 / 60 seconds.
        static constexpr float FRAME_SECONDS = 1.0f / FRAMES_PER_SECOND;

        bool enabled = false;
        bool paused = false;
        float step_seconds = 0.0f;
        // Only models sampled this frame need their frozen skinning matrices rebuilt.
        std::unordered_set<types::HandleId> stepped_models;
    };
} // namespace enishi::debug
