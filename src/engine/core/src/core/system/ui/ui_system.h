#pragma once
#include <core/system/interface_system.h>
#include <functional>
#include <memory>
#include <sub_system/ui/interface_ui.h>

namespace enishi::core {
    class UISystem final : public ISystem {
      public:
        using Content = std::function<void(sub_system::IUI&)>;
        using CloseCondition = std::function<bool(void)>;

      private:
        std::shared_ptr<sub_system::IUI> ui;
        Content content;
        CloseCondition close_condition;

      public:
        explicit UISystem(std::shared_ptr<sub_system::IUI> ui,
            Content content,
            CloseCondition close_condition = {});
        bool should_close(void) override;
        void pre_update(void) override;
        void update(const types::DeltaTime& delta_time) override;
        void post_update(void) override;
        void render(void) const override;
    };
} // namespace enishi::core
