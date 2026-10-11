#pragma once
#include <engine_types/assets/audio/audio_data.h>
#include <engine_types/assets/model/model_data.h>
#include <engine_types/assets/shader/shader_data.h>
#include <engine_types/assets/texture/texture_data.h>
#include <memory>
#include <variant>

namespace enishi::types {
    using AssetModelData = std::shared_ptr<ModelData>;
    using AssetShaderData = std::shared_ptr<ShaderData>;
    using AssetTextureData = std::shared_ptr<TextureData>;
    using AssetAudioData = std::shared_ptr<AudioData>;
    using AssetAnimationData = std::shared_ptr<int>;

    using AssetData = std::variant<AssetModelData,
        AssetShaderData,
        AssetTextureData,
        AssetAudioData,
        AssetAnimationData>;
} // namespace enishi::types
