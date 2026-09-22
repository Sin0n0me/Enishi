#pragma once
#include "pmx_model_loader.h"

namespace enishi::assets_system {
    class PMXReader;

    [[nodiscard]] foundation::Result<PMXData, AssetError> parse_pmx(
        std::span<const std::uint8_t> bytes);
} // namespace enishi::assets_system
