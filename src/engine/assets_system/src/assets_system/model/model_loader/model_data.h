#pragma once
#include "model_variant.h"
#include "pmd/pmd_data.h"
#include <foundation/option/option.h>
#include <variant>

namespace enishi::assets_system {
    class ModelData {
      private:
        ModelVariant model_data;

      public:
        template <typename T> foundation::Option<T*> get_data(void) {
            if (auto& data = std::get_if<std::unique_ptr<T>>(&this->get_data)) {
                return data.get();
            }
            return {};
        }

        template <typename T> const foundation::Option<const T*> get_data(void) const {
            if (auto& data = std::get_if<std::unique_ptr<T>>(&this->get_data)) {
                return data.get();
            }
            return {};
        }
    };
} // namespace enishi::assets_system