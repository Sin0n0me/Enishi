#include "obb_debug_system.h"
#include <component/model_component.h>
#include <debug/overlay/obb_wireframe.h>
#include <utility>

namespace enishi::core {
    OBBDebugSystem::OBBDebugSystem(std::shared_ptr<ecs::Registry> registry,
        std::shared_ptr<collider::Collider> collider,
        std::shared_ptr<platform::IWindow> window,
        std::shared_ptr<sub_system::ICameraController> camera,
        std::shared_ptr<debug::IDebugOverlay> overlay,
        std::shared_ptr<debug::PlaybackControl> playback)
        : registry(std::move(registry))
        , collider(std::move(collider))
        , window(std::move(window))
        , camera(std::move(camera))
        , overlay(std::move(overlay))
        , playback(std::move(playback)) {
    }

    void OBBDebugSystem::pre_update(void) {
        this->playback->step_seconds = 0.0f;
        this->playback->stepped_models.clear();
        const auto input = this->window->get_input();
        if (input.is_none()) {
            return;
        }
        if (input.unwrap()->is_key_pressed(types::KeyCode::KeyF3)) {
            this->playback->enabled = !this->playback->enabled;
        }
        if (!this->playback->enabled) {
            this->playback->paused = false;
            return;
        }
        if (input.unwrap()->is_key_pressed(types::KeyCode::KeySpace)) {
            this->playback->paused = !this->playback->paused;
        }
        if (this->playback->paused) {
            if (input.unwrap()->is_key_pressed(types::KeyCode::KeyLeft)) {
                this->playback->step_seconds -= debug::PlaybackControl::FRAME_SECONDS;
            }
            if (input.unwrap()->is_key_pressed(types::KeyCode::KeyRight)) {
                this->playback->step_seconds += debug::PlaybackControl::FRAME_SECONDS;
            }
        }
    }

    void OBBDebugSystem::update(const types::DeltaTime&) {
    }

    void OBBDebugSystem::post_update(void) {
        if (!this->playback->enabled) {
            return;
        }
        const auto camera = this->camera->get_camera();
        std::vector<debug::OverlayLine> lines;
        for (auto [entity, model] : this->registry->view<component::ModelComponent>()) {
            if (!model.visible) {
                continue;
            }
            const auto obbs = this->collider->get_obb_map(entity.handle_id);
            if (obbs == nullptr) {
                continue;
            }
            for (const auto& [bone, obb] : *obbs) {
                debug::append_obb_wireframe(obb, camera.mvp, lines);
            }
        }
        this->overlay->draw_lines(lines);
        this->overlay->draw_window_border();
    }

    void OBBDebugSystem::render(void) const {
    }
} // namespace enishi::core
