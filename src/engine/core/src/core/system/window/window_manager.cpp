#include "window_manager.h"

namespace enishi::core {
    WindowManager::WindowManager(std::shared_ptr<platform::IWindow> root_window)
        : root_window(root_window) {
    }

    std::weak_ptr<platform::IWindow> WindowManager::get_root_window(void) const {
        return this->root_window;
    }

    bool WindowManager::should_close(void) {
        return false;
    }

    void WindowManager::pre_update(void) {
        this->root_window->poll_events();
    }

    void WindowManager::update(const types::DeltaTime& delta_time) {
    }

    void WindowManager::post_update(void) {
    }

    void WindowManager::render(void) const {
    }
} // namespace enishi::core