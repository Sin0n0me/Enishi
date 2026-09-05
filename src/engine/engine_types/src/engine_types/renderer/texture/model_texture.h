#pragma once

namespace enishi::types {
    struct ModelTexture {
        // シェーダ側の名前と一致させる必要がある
        static constexpr char MODEL_TEXTURE_NAME[] = "model_texture";
        static constexpr char SPHERE_TEXTURE_NAME[] = "sphere_texture";
        static constexpr char TOON_TEXTURE_NAME[] = "toon_texture";
        static constexpr char MODEL_SAMPLER_NAME[] = "model_sampler";
        static constexpr char SPHERE_SAMPLER_NAME[] = "sphere_sampler";
        static constexpr char TOON_SAMPLER_NAME[] = "toon_sampler";
    };
} // namespace enishi::types