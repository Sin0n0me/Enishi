#pragma once
#include <foundation/result/result.h>
#include <string>
#include <vector>

namespace enishi::sub_system {
    struct ModelEntry {
        std::string id;
        std::string filename;
        bool visible;
    };

    enum class ModelSelectionError { LoadFailed, RegistrationFailed };

    class IModelSelection {
      public:
        virtual ~IModelSelection(void) noexcept = default;
        [[nodiscard]] virtual std::vector<ModelEntry> get_models(void) const = 0;
        [[nodiscard]] virtual foundation::VoidResult<ModelSelectionError> show_model(
            const std::string& id) = 0;
        virtual void hide_model(const std::string& id) = 0;
    };
} // namespace enishi::sub_system
