#pragma once
#include <collider/collider.h>
#include <core/system/interface_system.h>
#include <platform/window/interface_window.h>
#include <sub_system/collision/interface_model_click_handler.h>

namespace enishi::core {
    class ModelClickSystem : public ISystem {
      private:
        std::shared_ptr<collider::Collider> collider;
        std::shared_ptr<platform::IWindow> window;
        std::vector<std::shared_ptr<sub_system::IModelClickHandler>> handlers;

      public:
        ModelClickSystem(std::shared_ptr<collider::Collider> collider,
            std::shared_ptr<platform::IWindow> window);
        void add_handler(std::shared_ptr<sub_system::IModelClickHandler> handler);
        [[nodiscard]] std::optional<types::ModelHit> pick(types::ClientMousePosition cursor) const;
        bool should_close(void) override;
        void pre_update(void) override;
        void update(const types::DeltaTime& delta_time) override;
        void post_update(void) override;
        void render(void) const override;
    };
} // namespace enishi::core
