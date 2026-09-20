#pragma once

#include <cstdint>
#include <unordered_map>

namespace enishi::renderer::opengl {
    class TextureUnitBinder {
      private:
        std::unordered_map<std::uint32_t, std::uint32_t> textures;

      public:
        void bind(std::uint32_t binding, std::uint32_t texture) noexcept;
        [[nodiscard]] std::uint32_t get(std::uint32_t binding) const noexcept;
    };
} // namespace enishi::renderer::opengl
