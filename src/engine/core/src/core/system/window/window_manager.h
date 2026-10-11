#pragma once
#include "../interface_system.h"
#include <memory>
#include <platform/window/interface_window.h>
#include <vector>

namespace enishi::core {
    class WindowManager : public ISystem {
      private:
        std::shared_ptr<platform::IWindow> root_window;

      public:
        explicit WindowManager(std::shared_ptr<platform::IWindow> root_window);

      public:
        std::weak_ptr<platform::IWindow> get_root_window(void) const;

      public:
        bool should_close(void) override;
        void pre_update(void) override;
        void update(const types::DeltaTime& delta_time) override;
        void post_update(void) override;
        void render(void) const override;
    };
} // namespace enishi::core