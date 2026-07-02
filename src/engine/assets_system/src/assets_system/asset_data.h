#pragma once
#include <engine_types/assets/model/model_data.h>
#include <engine_types/assets/shader/shader_data.h>
#include <engine_types/assets/texture/texture_data.h>
#include <variant>

namespace enishi::assets_system {
    using AssetData =
        std::variant<std::monostate, types::ModelData, types::ShaderData, types::TextureData>;
} // namespace enishi::assets_system
