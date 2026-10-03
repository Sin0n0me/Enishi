#pragma once
#include "../../errors/errors.h"
#include "model_variant.h"
#include <filesystem>
#include <foundation/result/result.h>
#include <foundation/str/str.h>
#include <memory>

namespace enishi::assets_system {
    class IModelLoader {
      public:
        virtual ~IModelLoader(void) noexcept = default;

        [[nodiscard]] virtual foundation::Result<ModelVariant, AssetError> load(
            const std::filesystem::path& path) noexcept = 0;

        [[nodiscard]] virtual foundation::UTF8 get_supported_extension(void) const noexcept = 0;
    };
} // namespace enishi::assets_system