#pragma once
#include "../interface_model_loader.h"

namespace enishi::assets_system {
    class VRMModelLoader : public IModelLoader {
      public:
        foundation::Result<ModelVariant, AssetError> load(
            const std::filesystem::path& path) noexcept override;
        foundation::UTF8 get_supported_extension() const noexcept override;
    };
} // namespace enishi::assets_system
