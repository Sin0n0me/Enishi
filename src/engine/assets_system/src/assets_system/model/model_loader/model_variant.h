#pragma once
#include "pmd/pmd_data.h"
#include <foundation/option/option.h>
#include <variant>

namespace enishi::assets_system {
    using ModelVariant = std::variant<std::monostate, std::unique_ptr<PMDData>>;
} // namespace enishi::assets_system