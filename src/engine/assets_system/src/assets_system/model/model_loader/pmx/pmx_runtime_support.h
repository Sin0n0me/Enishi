#pragma once
#include "../../../errors/errors.h"
#include "pmx_data.h"

namespace enishi::assets_system {
    // Structural validity and runtime support are separate: PMXData retains unsupported data.
    [[nodiscard]] foundation::Result<void, AssetError> validate_pmx_runtime_support(
        const PMXData& data);
} // namespace enishi::assets_system
