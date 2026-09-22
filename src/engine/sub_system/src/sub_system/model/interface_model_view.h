#pragma once
#include <glm/glm.hpp>

namespace enishi::sub_system {
    class IModelView {
      public:
        virtual ~IModelView(void) noexcept = default;
    };
} // namespace enishi::sub_system