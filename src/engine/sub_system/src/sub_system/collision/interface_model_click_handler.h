#pragma once
#include <engine_types/collider/model_click.h>

namespace enishi::sub_system {
    class IModelClickHandler {
      public:
        virtual ~IModelClickHandler(void) noexcept = default;
        // A missing hit denotes a click that did not touch a visible model.
        virtual void on_model_click(const types::ModelClick& event) = 0;
    };
} // namespace enishi::sub_system
