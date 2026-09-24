#pragma once
#include "../interface_model_loader.h"
#include "pmx_data.h"
#include <span>

namespace enishi::assets_system {
    class PMXModelLoader : public IModelLoader {
      public:
        foundation::Result<ModelVariant, AssetError> load(
            const std::filesystem::path& path) noexcept override;
        foundation::UTF8 get_supported_extension(void) const noexcept override;

        [[nodiscard]] static foundation::Result<PMXData, AssetError> parse(
            std::span<const std::uint8_t> bytes);
        [[nodiscard]] static foundation::Result<void, AssetError> validate(const PMXData& data);
    };
} // namespace enishi::assets_system
