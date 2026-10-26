#include "system_scheduler.h"

namespace enishi::core {
    void SystemScheduler::pre_update(void) {
        for (auto& entry : this->managed_systems) {
            entry.system->pre_update();
        }
    }

    void SystemScheduler::update(const types::DeltaTime& dt) {
        for (auto& entry : this->managed_systems) {
            entry.system->update(dt);
        }
    }

    void SystemScheduler::post_update(void) {
        for (auto& entry : this->managed_systems) {
            entry.system->post_update();
        }
    }

    void SystemScheduler::render(void) const {
        for (auto& entry : this->managed_systems) {
            entry.system->render();
        }
    }
} // namespace enishi::core